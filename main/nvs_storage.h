#pragma once
#include "esp_err.h"
#include <stdint.h>

#define MAX_UID_LEN    16
#define MAX_NAME_LEN   32
#define MAX_USERS      50

// Estrutura de configuração de rede
typedef struct {
    char ssid[32];
    char pass[64];
} wifi_config_t;

// Estrutura de configuração Zigbee
typedef struct {
    uint8_t channel;
    uint16_t pan_id;
} zigbee_config_t;

// Estrutura de usuário RFID
typedef struct {
    char uid[MAX_UID_LEN];
    char nome[MAX_NAME_LEN];
} rfid_user_t;

// API
esp_err_t nvs_storage_init(void);

// WiFi
esp_err_t nvs_save_wifi(const wifi_config_t *cfg);
esp_err_t nvs_load_wifi(wifi_config_t *cfg);

// Zigbee
esp_err_t nvs_save_zigbee(const zigbee_config_t *cfg);
esp_err_t nvs_load_zigbee(zigbee_config_t *cfg);

// RFID Users
esp_err_t nvs_add_user(const rfid_user_t *user);
esp_err_t nvs_list_users(rfid_user_t *users, size_t *count);
esp_err_t nvs_clear_users(void);
