#include "rfid_reader.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "RFID_WIEGAND";

// Pinos de entrada (ajusta conforme tua placa ESP32-C6)
#define WIEGAND_D0_PIN  4
#define WIEGAND_D1_PIN  5

// Buffer para armazenar bits
#define WIEGAND_MAX_BITS 64
static uint8_t wiegand_bits[WIEGAND_MAX_BITS];
static volatile int bit_count = 0;

// Fila para sinalizar bits
static QueueHandle_t wiegand_queue = NULL;

// Lista de UIDs autorizados
static const char *authorized_uids[] = {
    "123456",   // Exemplo
    "987654",   // Exemplo
};

typedef struct {
    int bit;
} wiegand_event_t;

// ISR D0
static void IRAM_ATTR wiegand_isr_d0(void *arg) {
    wiegand_event_t evt = { .bit = 0 };
    xQueueSendFromISR(wiegand_queue, &evt, NULL);
}

// ISR D1
static void IRAM_ATTR wiegand_isr_d1(void *arg) {
    wiegand_event_t evt = { .bit = 1 };
    xQueueSendFromISR(wiegand_queue, &evt, NULL);
}

// Task para reconstruir o UID
static void wiegand_task(void *arg) {
    wiegand_event_t evt;
    while (1) {
        if (xQueueReceive(wiegand_queue, &evt, portMAX_DELAY)) {
            if (bit_count < WIEGAND_MAX_BITS) {
                wiegand_bits[bit_count++] = evt.bit;
            }

            // Timeout para considerar fim da transmissão
            vTaskDelay(pdMS_TO_TICKS(30));

            if (bit_count > 0) {
                char uid_str[32];
                uint32_t uid_val = 0;

                for (int i = 0; i < bit_count; i++) {
                    uid_val = (uid_val << 1) | wiegand_bits[i];
                }

                snprintf(uid_str, sizeof(uid_str), "%u", uid_val);
                ESP_LOGI(TAG, "UID recebido: %s", uid_str);

                // Reset contador
                bit_count = 0;
            }
        }
    }
}

esp_err_t rfid_reader_init(void) {
    ESP_LOGI(TAG, "Inicializando leitor RFID Wiegand...");

    wiegand_queue = xQueueCreate(32, sizeof(wiegand_event_t));
    if (!wiegand_queue) {
        return ESP_FAIL;
    }

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << WIEGAND_D0_PIN) | (1ULL << WIEGAND_D1_PIN),
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    gpio_isr_handler_add(WIEGAND_D0_PIN, wiegand_isr_d0, NULL);
    gpio_isr_handler_add(WIEGAND_D1_PIN, wiegand_isr_d1, NULL);

    xTaskCreate(wiegand_task, "wiegand_task", 4096, NULL, 10, NULL);

    return ESP_OK;
}

esp_err_t rfid_read_uid(char *uid_str, size_t uid_size) {
    // TODO: Pode implementar buffer compartilhado para fornecer UID ao main
    // por enquanto devolve ESP_FAIL porque UID já é logado no task
    return ESP_FAIL;
}

bool rfid_is_user_authorized(const char *uid) {
    for (size_t i = 0; i < sizeof(authorized_uids)/sizeof(authorized_uids[0]); i++) {
        if (strcmp(uid, authorized_uids[i]) == 0) {
            return true;
        }
    }
    return false;
}
