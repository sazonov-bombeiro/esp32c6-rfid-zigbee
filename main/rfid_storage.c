#include "rfid_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>
#include <stdio.h>

#define STORAGE_NAMESPACE "rfid_storage"
#define KEY_USERS   "users"
#define KEY_LOGS    "logs"

// Banco em RAM
static rfid_user_t user_db[MAX_USERS];
static int user_count = 0;

static rfid_log_t log_db[MAX_LOGS];
static int log_count = 0;

// ====================== Funções internas ======================

static esp_err_t save_users_to_nvs() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, KEY_USERS, user_db, sizeof(user_db));
    if (err == ESP_OK) {
        err = nvs_set_i32(handle, "user_count", user_count);
    }

    nvs_commit(handle);
    nvs_close(handle);
    return err;
}

static esp_err_t load_users_from_nvs() {
    nvs_handle_t handle;
    size_t required_size = sizeof(user_db);
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    err = nvs_get_blob(handle, KEY_USERS, user_db, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return ESP_OK; // vazio
    }

    int32_t count = 0;
    if (nvs_get_i32(handle, "user_count", &count) == ESP_OK) {
        user_count = count;
    }

    nvs_close(handle);
    return err;
}

static esp_err_t save_logs_to_nvs() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, KEY_LOGS, log_db, sizeof(log_db));
    if (err == ESP_OK) {
        err = nvs_set_i32(handle, "log_count", log_count);
    }

    nvs_commit(handle);
    nvs_close(handle);
    return err;
}

static esp_err_t load_logs_from_nvs() {
    nvs_handle_t handle;
    size_t required_size = sizeof(log_db);
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    err = nvs_get_blob(handle, KEY_LOGS, log_db, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return ESP_OK; // vazio
    }

    int32_t count = 0;
    if (nvs_get_i32(handle, "log_count", &count) == ESP_OK) {
        log_count = count;
    }

    nvs_close(handle);
    return err;
}

// ====================== API pública ======================

esp_err_t rfid_storage_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    // Carrega dados
    load_users_from_nvs();
    load_logs_from_nvs();
    return ESP_OK;
}

int rfid_get_logs(rfid_log_t **logs) {
    *logs = log_db;
    return log_count;
}

int rfid_list_users(rfid_user_t **users) {
    *users = user_db;
    return user_count;
}

esp_err_t rfid_add_user(const char *uid, const char *name) {
    if (user_count >= MAX_USERS) {
        return ESP_ERR_NO_MEM;
    }

    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_db[i].uid, uid) == 0) {
            return ESP_ERR_INVALID_STATE; // já cadastrado
        }
    }

    strncpy(user_db[user_count].uid, uid, sizeof(user_db[user_count].uid) - 1);
    strncpy(user_db[user_count].name, name, sizeof(user_db[user_count].name) - 1);
    user_count++;

    return save_users_to_nvs();
}

esp_err_t rfid_remove_user(const char *uid) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_db[i].uid, uid) == 0) {
            user_db[i] = user_db[user_count - 1];
            user_count--;
            return save_users_to_nvs();
        }
    }
    return ESP_ERR_NOT_FOUND;
}

bool rfid_is_user_authorized(const char *uid) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_db[i].uid, uid) == 0) {
            return true;
        }
    }
    return false;
}

esp_err_t rfid_add_log(const char *uid, const char *timestamp) {
    if (log_count >= MAX_LOGS) {
        // FIFO - remove o mais antigo
        for (int i = 1; i < MAX_LOGS; i++) {
            log_db[i - 1] = log_db[i];
        }
        log_count = MAX_LOGS - 1;
    }

    strncpy(log_db[log_count].uid, uid, sizeof(log_db[log_count].uid) - 1);
    strncpy(log_db[log_count].timestamp, timestamp, sizeof(log_db[log_count].timestamp) - 1);
    log_count++;

    return save_logs_to_nvs();
}

