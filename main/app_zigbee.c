#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

// ===== ВНИМАНИЕ =====
// Для Zigbee используются заголовки из ESP-Zigbee-SDK.
// Убедитесь, что EXTRA_COMPONENT_DIRS указывает на components SDK.
#include "esp_zigbee_core.h"
#include "esp_zigbee_zcl.h"

#include "app_zigbee.h"

#define APP_ENDPOINT             10
#define APP_PROFILE_ID           ESP_ZB_AF_HA_PROFILE_ID
#define CLUSTER_CUSTOM_ID        0xFC00
#define ATTR_LAST_UID_ID         0x0001

static const char *TAG = "ZB";

// ---------
// Хранение последнего UID как строка ZCL (первый байт длина)
// ---------
static uint8_t last_uid_zcl[1 + 16] = {0};

// ---------- Вспомогательные билдеры для эндпоинта ----------
static esp_zb_cluster_list_t *build_cluster_list(void)
{
    // Basic cluster (минимальная информация об устройстве)
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = 3,
    };
    esp_zb_attribute_list_t *basic = esp_zb_basic_cluster_create(&basic_cfg);

    // Identify cluster (возможность "мигать" при поиске устройства)
    esp_zb_identify_cluster_cfg_t identify_cfg = {
        .identify_time = 0,
    };
    esp_zb_attribute_list_t *identify = esp_zb_identify_cluster_create(&identify_cfg);

    // Custom cluster (диапазон >0x8000 допустим для вендорских расширений)
    esp_zb_attribute_list_t *custom = esp_zb_zcl_attr_list_create(CLUSTER_CUSTOM_ID);
    esp_zb_zcl_attr_t last_uid_attr = {
        .id = ATTR_LAST_UID_ID,
        .type = ESP_ZB_ZCL_ATTR_TYPE_CHAR_STRING,
        .access = ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
        .data_p = &last_uid_zcl[0],
    };
    esp_zb_zcl_attr_list_add_attr(custom, &last_uid_attr);

    esp_zb_cluster_list_t *cluster_list = esp_zb_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(cluster_list, basic, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(cluster_list, identify, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_custom_cluster(cluster_list, custom, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    return cluster_list;
}

static esp_zb_ep_list_t *build_endpoint_list(void)
{
    esp_zb_endpoint_config_t ep_config = {
        .endpoint = APP_ENDPOINT,
        .app_profile_id = APP_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_CUSTOM_ATTR_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_ep_list_add_ep(ep_list, build_cluster_list(), ep_config);
    return ep_list;
}

// --------- Инициализация Zigbee ---------
void app_zb_init_start(void)
{
    ESP_LOGI(TAG, "Init Zigbee stack (Router)");

    // Конфиг сети: роутер (для ZHA обычно удобно)
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZR_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    // Каналы: разрешим все, координатор выберет
    esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK);

    esp_zb_device_register(build_endpoint_list());
    esp_zb_start(true);
}

// --------- Отправка отчета об атрибуте при считывании карты ---------
void app_zb_report_uid(const uint8_t *uid, size_t uid_len)
{
    if (!uid) return;
    if (uid_len > 16) uid_len = 16;

    last_uid_zcl[0] = (uint8_t)uid_len;
    memcpy(&last_uid_zcl[1], uid, uid_len);

    // Локально обновим значение атрибута
    esp_zb_zcl_set_attribute_val(APP_ENDPOINT, CLUSTER_CUSTOM_ID,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ATTR_LAST_UID_ID, last_uid_zcl, true);

    // Отправим отчет координатору (обычно короткий адрес 0x0000, endpoint 1 у ZHA)
    esp_zb_zcl_report_attr_cmd_t cmd = {
        .dst_addr_u.addr_short = 0x0000,
        .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        .dst_endpoint = 1,
        .src_endpoint = APP_ENDPOINT,
        .clusterID = CLUSTER_CUSTOM_ID,
        .attrID = ATTR_LAST_UID_ID,
    };
    esp_zb_zcl_report_attr_cmd_req(&cmd);

    ESP_LOGI(TAG, "UID reported via Zigbee (%d bytes)", (int)uid_len);
}
