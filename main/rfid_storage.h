#ifndef RFID_STORAGE_H
#define RFID_STORAGE_H

#include "esp_err.h"

#define MAX_UID_LEN      32
#define MAX_NAME_LEN     64
#define MAX_TIMESTAMP_LEN 32
#define MAX_USERS         50
#define MAX_LOGS          50

// Estrutura de usuário
typedef struct {
    char uid[MAX_UID_LEN];
    char name[MAX_NAME_LEN];
} rfid_user_t;

// Estrutura de log
typedef struct {
    char uid[MAX_UID_LEN];
    char timestamp[MAX_TIMESTAMP_LEN];
} rfid_log_t;

// Inicialização
esp_err_t rfid_storage_init(void);

// Usuários
esp_err_t rfid_add_user(const char *uid, const char *name);
esp_err_t rfid_remove_user(const char *uid);
int rfid_list_users(rfid_user_t **users);

// Logs
esp_err_t rfid_add_log(const char *uid, const char *timestamp);
int rfid_get_logs(rfid_log_t **logs);

#endif // RFID_STORAGE_H
