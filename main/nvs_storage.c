#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "nvs_storage.h"

static const char *TAG = "nvs_storage";

// =====================
// Inicialização
// =====================
esp_err_t nvs_storage_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_LOGI(TAG, "NVS inicializado");
    return ret;
}

// =====================
// WiFi Config
// =====================
esp_err_t nvs_save_wifi(const wifi_config_t *cfg)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    nvs_set_str(handle, "wifi_ssid", cfg->ssid);
    nvs_set_str(handle, "wifi_pass", cfg->pass);

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t nvs_load_wifi(wifi_config_t *cfg)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    size_t len = sizeof(cfg->ssid);
    err = nvs_get_str(handle, "wifi_ssid", cfg->ssid, &len);
    if (err != ESP_OK) { nvs_close(handle); return err; }

    len = sizeof(cfg->pass);
    err = nvs_get_str(handle, "wifi_pass", cfg->pass, &len);

    nvs_close(handle);
    return err;
}

// =====================
// Zigbee Config
// =====================
esp_err_t nvs_save_zigbee(const zigbee_config_t *cfg)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    nvs_set_u8(handle, "zb_chan", cfg->channel);
    nvs_set_u16(handle, "zb_panid", cfg->pan_id);

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t nvs_load_zigbee(zigbee_config_t *cfg)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    err = nvs_get_u8(handle, "zb_chan", &cfg->channel);
    if (err != ESP_OK) { nvs_close(handle); return err; }

    err = nvs_get_u16(handle, "zb_panid", &cfg->pan_id);

    nvs_close(handle);
    return err;
}

// =====================
// RFID Users
// =====================
esp_err_t nvs_add_user(const rfid_user_t *user)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("users", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    // Recupera contador atual
    uint32_t count = 0;
    nvs_get_u32(handle, "count", &count);

    if (count >= MAX_USERS) {
        nvs_close(handle);
        return ESP_ERR_NO_MEM;
    }

    char key_uid[16], key_nome[16];
    sprintf(key_uid, "uid_%lu", count);
    sprintf(key_nome, "nome_%lu", count);

    nvs_set_str(handle, key_uid, user->uid);
    nvs_set_str(handle, key_nome, user->nome);

    nvs_set_u32(handle, "count", count + 1);

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t nvs_list_users(rfid_user_t *users, size_t *count)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("users", NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    uint32_t saved_count = 0;
    nvs_get_u32(handle, "count", &saved_count);
    if (saved_count > *count) saved_count = *count;

    for (uint32_t i = 0; i < saved_count; i++) {
        char key_uid[16], key_nome[16];
        sprintf(key_uid, "uid_%u", i);
        sprintf(key_nome, "nome_%u", i);

        size_t len_uid = MAX_UID_LEN;
        size_t len_nome = MAX_NAME_LEN;
        nvs_get_str(handle, key_uid, users[i].uid, &len_uid);
        nvs_get_str(handle, key_nome, users[i].nome, &len_nome);
    }

    *count = saved_count;
    nvs_close(handle);
    return ESP_OK;
}

esp_err_t nvs_clear_users(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("users", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}
