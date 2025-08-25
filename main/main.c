#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "rfid_storage.h"
#include "rfid_reader.h"

// Zigbee
#include "esp_zigbee_core.h"
#include "esp_zigbee_platform.h"
#include "esp_zigbee_type.h"
// #include "zcl/esp_zigbee_zcl.h" // se for usar clusters

static const char *TAG = "APP_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando armazenamento NVS...");
    ESP_ERROR_CHECK(rfid_storage_init());

    ESP_LOGI(TAG, "Inicializando leitor RFID...");
    ESP_ERROR_CHECK(rfid_reader_init());

    ESP_LOGI(TAG, "Inicializando Zigbee...");

    // Configuração de rádio + host
    esp_zb_platform_config_t platform_config = {
        .radio_config = { ESP_ZB_DEFAULT_RADIO_CONFIG() },
        .host_config  = { ESP_ZB_DEFAULT_HOST_CONFIG() },
    };
    ESP_ERROR_CHECK(esp_zb_platform_init(&platform_config));

    // Configuração do dispositivo como ROUTER
    esp_zb_cfg_t zb_config = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,
        .nwk_cfg     = { ESP_ZB_DEFAULT_NETWORK_CONFIG() },
    };
    esp_zb_init(&zb_config);  // sem ESP_ERROR_CHECK

    ESP_LOGI(TAG, "Entrando no loop principal Zigbee...");

    while (true) {
        // Iteração do loop Zigbee
        esp_zb_main_loop_iteration();

        // Aqui você pode verificar RFID periodicamente
        char uid[32];
        if (rfid_read_uid(uid, sizeof(uid)) == ESP_OK) {
            if (rfid_is_user_authorized(uid)) {
                ESP_LOGI(TAG, "UID autorizado: %s", uid);
                rfid_add_log(uid, "2025-08-25 12:00");  // timestamp de exemplo
            } else {
                ESP_LOGW(TAG, "UID não autorizado: %s", uid);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
