#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "app_wiegand.h"
#include "app_zigbee.h"

// ---------------------------
// Реализация протокола Wiegand (D0/D1)
// Подход: прерывания по фронту на D0/D1 добавляют биты (0 или 1).
// Таймер "тишины" завершает кадр (обычно 30..50 мс).
// ---------------------------

#define WG_MAX_BITS     64            // поддержим до 64 бит (26..58 типично)
#define WG_FRAME_GAP_US 40000         // 40 мс таймаут между битами

static const char *TAG = "WIEGAND";
static wiegand_pins_t wg_pins;

// Буфер для бит
static uint8_t bit_buf[WG_MAX_BITS] = {0};
static volatile int bit_count = 0;

// UID последней карты (не парсим поля, просто собираем как байты)
static uint8_t last_uid[16] = {0};
static size_t last_uid_len = 0;

static esp_timer_handle_t frame_timer;

// --- Прототипы ---
static void IRAM_ATTR isr_d0(void* arg);
static void IRAM_ATTR isr_d1(void* arg);
static void frame_timeout(void* arg);
static void apply_relay_pulse(void);

// ---------------- Инициализация ----------------
esp_err_t wiegand_init(const wiegand_pins_t *pins)
{
    if (!pins) return ESP_ERR_INVALID_ARG;
    wg_pins = *pins;

    // Конфиг входов D0/D1
    gpio_config_t in_cfg = {
        .pin_bit_mask = (1ULL<<wg_pins.gpio_d0) | (1ULL<<wg_pins.gpio_d1),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE, // большинство считывателей тянут линию в 0 коротким импульсом
    };
    ESP_ERROR_CHECK(gpio_config(&in_cfg));

    // Выходы: LED/BUZZER/RELAY
    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL<<wg_pins.gpio_led) | (1ULL<<wg_pins.gpio_buzzer) | (1ULL<<wg_pins.gpio_relay),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&out_cfg));
    gpio_set_level(wg_pins.gpio_led, 0);
    gpio_set_level(wg_pins.gpio_buzzer, 0);
    gpio_set_level(wg_pins.gpio_relay, 0);

    // Таймер завершения кадра
    const esp_timer_create_args_t targs = {
        .callback = &frame_timeout,
        .name = "wg_frame",
        .arg = NULL
    };
    ESP_ERROR_CHECK(esp_timer_create(&targs, &frame_timer));

    // Прерывания
    ESP_ERROR_CHECK(gpio_isr_handler_add(wg_pins.gpio_d0, isr_d0, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(wg_pins.gpio_d1, isr_d1, NULL));

    ESP_LOGI(TAG, "Wiegand initialized: D0=%d D1=%d LED=%d BUZ=%d RELAY=%d",
             wg_pins.gpio_d0, wg_pins.gpio_d1, wg_pins.gpio_led, wg_pins.gpio_buzzer, wg_pins.gpio_relay);
    return ESP_OK;
}

// ----------- Обработчики прерываний -----------
static inline void wg_push_bit(uint8_t v)
{
    if (bit_count < WG_MAX_BITS) {
        bit_buf[bit_count++] = (v ? 1 : 0);
    }
    // перезапускаем таймер "конца кадра"
    esp_timer_stop(frame_timer);
    esp_timer_start_once(frame_timer, WG_FRAME_GAP_US);
}

static void IRAM_ATTR isr_d0(void* arg)
{
    // Импульс на D0 обозначает бит '0'
    wg_push_bit(0);
}

static void IRAM_ATTR isr_d1(void* arg)
{
    // Импульс на D1 обозначает бит '1'
    wg_push_bit(1);
}

// ---------- Когда кадр завершился ----------
static void frame_timeout(void* arg)
{
    // простая упаковка бит в байты слева направо
    uint8_t bytes[16] = {0};
    int nbits = bit_count;
    int nbytes = (nbits + 7) / 8;
    if (nbytes > 16) nbytes = 16;

    for (int i = 0; i < nbits && i < 128; ++i) {
        int byte_index = i / 8;
        int bit_index = 7 - (i % 8); // MSB-first
        if (bit_buf[i]) bytes[byte_index] |= (1 << bit_index);
    }

    // сохраним как "последний UID"
    memcpy(last_uid, bytes, nbytes);
    last_uid_len = nbytes;

    // индикация (мигнем светодиодом/бип)
    gpio_set_level(wg_pins.gpio_led, 1);
    gpio_set_level(wg_pins.gpio_buzzer, 1);
    apply_relay_pulse();

    // Сообщим в Zigbee
    app_zigbee_report_uid(last_uid, last_uid_len);

    // сброс
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(wg_pins.gpio_led, 0);
    gpio_set_level(wg_pins.gpio_buzzer, 0);

    bit_count = 0;
    memset(bit_buf, 0, sizeof(bit_buf));
}

static void apply_relay_pulse(void)
{
    // простейший импульс 500 мс
    gpio_set_level(wg_pins.gpio_relay, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(wg_pins.gpio_relay, 0);
}

// ---------- API ----------
void wiegand_get_last_uid(uint8_t *buf, size_t *len)
{
    if (!buf || !len) return;
    if (*len < last_uid_len) {
        *len = 0;
        return;
    }
    memcpy(buf, last_uid, last_uid_len);
    *len = last_uid_len;
}

void wiegand_clear_last_uid(void)
{
    memset(last_uid, 0, sizeof(last_uid));
    last_uid_len = 0;
}
