#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "rfid_reader.h"
#include "rfid_storage.h"

static const char *TAG = "app_web";
static httpd_handle_t server = NULL;

/* ------------------- HANDLERS ------------------- */

// Página principal
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char resp[] =
        "<h1>ESP32C6 RFID + Zigbee</h1>"
        "<p><a href=\"/config\">Config Zigbee</a></p>"
        "<p><a href=\"/rfid_logs\">RFID Logs</a></p>"
        "<p><a href=\"/users\">Users</a></p>"
        "<p><a href=\"/manage_users\">Gerir Usuários</a></p>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Página de configuração Zigbee (placeholder)
static esp_err_t config_get_handler(httpd_req_t *req)
{
    const char resp[] = "<h1>Config Zigbee</h1><p>Futuro formulário de rede...</p>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Listar logs RFID
static esp_err_t logs_get_handler(httpd_req_t *req)
{
    rfid_log_t *logs;
    int count = rfid_get_logs(&logs);

    httpd_resp_sendstr_chunk(req, "<h1>RFID Logs</h1><ul>");
    for (int i = 0; i < count; i++) {
        char line[128];
        snprintf(line, sizeof(line), "<li>UID: %s | Time: %s</li>", logs[i].uid, logs[i].timestamp);
        httpd_resp_sendstr_chunk(req, line);
    }
    httpd_resp_sendstr_chunk(req, "</ul>");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

// Listar usuários
static esp_err_t users_get_handler(httpd_req_t *req)
{
    rfid_user_t *users;
    int count = rfid_list_users(&users);

    httpd_resp_sendstr_chunk(req, "<h1>Usuários</h1><ul>");
    for (int i = 0; i < count; i++) {
        char line[128];
        snprintf(line, sizeof(line), "<li>%s (UID=%s)</li>", users[i].name, users[i].uid);
        httpd_resp_sendstr_chunk(req, line);
    }
    httpd_resp_sendstr_chunk(req, "</ul>");
    httpd_resp_sendstr_chunk(req, "<p><a href=\"/manage_users\">Gerir Usuários</a></p>");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

// Formulário de gestão de usuários
static esp_err_t manage_users_handler(httpd_req_t *req)
{
    const char resp[] =
        "<h1>Gerir Usuários RFID</h1>"
        "<h2>Adicionar Usuário</h2>"
        "<form action=\"/add_user\" method=\"get\">"
        "UID: <input type=\"text\" name=\"uid\"><br>"
        "Nome: <input type=\"text\" name=\"name\"><br>"
        "<input type=\"submit\" value=\"Adicionar\">"
        "</form>"
        "<h2>Remover Usuário</h2>"
        "<form action=\"/remove_user\" method=\"get\">"
        "UID: <input type=\"text\" name=\"uid\"><br>"
        "<input type=\"submit\" value=\"Remover\">"
        "</form>"
        "<p><a href=\"/users\">Ver lista de usuários</a></p>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Adicionar usuário
static esp_err_t add_user_handler(httpd_req_t *req)
{
    char buf[128];
    int ret = httpd_req_get_url_query_str(req, buf, sizeof(buf));
    if (ret == ESP_OK) {
        char uid[32] = {0}, name[32] = {0};
        httpd_query_key_value(buf, "uid", uid, sizeof(uid));
        httpd_query_key_value(buf, "name", name, sizeof(name));
        if (strlen(uid) > 0 && strlen(name) > 0) {
            if (rfid_add_user(uid, name) == ESP_OK) {
                httpd_resp_sendstr(req, "Usuário adicionado com sucesso.<br><a href=\"/users\">Voltar</a>");
            } else {
                httpd_resp_sendstr(req, "Erro ao adicionar usuário.<br><a href=\"/manage_users\">Voltar</a>");
            }
        } else {
            httpd_resp_sendstr(req, "Parâmetros inválidos.<br><a href=\"/manage_users\">Voltar</a>");
        }
    } else {
        httpd_resp_sendstr(req, "Erro ao ler parâmetros.<br><a href=\"/manage_users\">Voltar</a>");
    }
    return ESP_OK;
}

// Remover usuário
static esp_err_t remove_user_handler(httpd_req_t *req)
{
    char buf[128];
    int ret = httpd_req_get_url_query_str(req, buf, sizeof(buf));
    if (ret == ESP_OK) {
        char uid[32] = {0};
        httpd_query_key_value(buf, "uid", uid, sizeof(uid));
        if (strlen(uid) > 0) {
            if (rfid_remove_user(uid) == ESP_OK) {
                httpd_resp_sendstr(req, "Usuário removido com sucesso.<br><a href=\"/users\">Voltar</a>");
            } else {
                httpd_resp_sendstr(req, "UID não encontrado.<br><a href=\"/manage_users\">Voltar</a>");
            }
        } else {
            httpd_resp_sendstr(req, "Parâmetros inválidos.<br><a href=\"/manage_users\">Voltar</a>");
        }
    } else {
        httpd_resp_sendstr(req, "Erro ao ler parâmetros.<br><a href=\"/manage_users\">Voltar</a>");
    }
    return ESP_OK;
}

/* ------------------- SERVER START ------------------- */
httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        // Rotas
        httpd_uri_t uri_root   = { .uri = "/",            .method = HTTP_GET, .handler = root_get_handler };
        httpd_uri_t uri_cfg    = { .uri = "/config",      .method = HTTP_GET, .handler = config_get_handler };
        httpd_uri_t uri_logs   = { .uri = "/rfid_logs",   .method = HTTP_GET, .handler = logs_get_handler };
        httpd_uri_t uri_users  = { .uri = "/users",       .method = HTTP_GET, .handler = users_get_handler };
        httpd_uri_t uri_manage = { .uri = "/manage_users",.method = HTTP_GET, .handler = manage_users_handler };
        httpd_uri_t uri_add    = { .uri = "/add_user",    .method = HTTP_GET, .handler = add_user_handler };
        httpd_uri_t uri_remove = { .uri = "/remove_user", .method = HTTP_GET, .handler = remove_user_handler };

        httpd_register_uri_handler(server, &uri_root);
        httpd_register_uri_handler(server, &uri_cfg);
        httpd_register_uri_handler(server, &uri_logs);
        httpd_register_uri_handler(server, &uri_users);
        httpd_register_uri_handler(server, &uri_manage);
        httpd_register_uri_handler(server, &uri_add);
        httpd_register_uri_handler(server, &uri_remove);

        ESP_LOGI(TAG, "Servidor HTTP iniciado");
    }
    return server;
}
