#pragma once
#include "esp_http_server.h"   // <--- ESSENCIAL p/ httpd_req_t
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicia o servidor web principal
 */
void start_webserver(void);

/**
 * @brief Handler da página de configuração de rede/Zigbee
 *
 * Normalmente responde a /config
 */
esp_err_t web_handle_config_get(httpd_req_t *req);
esp_err_t web_handle_config_post(httpd_req_t *req);

/**
 * @brief Handler da página de logs dos cartões RFID lidos
 *
 * Normalmente responde a /logs
 */
esp_err_t web_handle_logs(httpd_req_t *req);

/**
 * @brief Handler da página de configuração de usuários RFID
 *
 * Normalmente responde a /users
 */
esp_err_t web_handle_users_get(httpd_req_t *req);
esp_err_t web_handle_users_post(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
