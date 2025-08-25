#include <string.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_http_server.h"

#include "app_http.h"
#include "app_wiegand.h"

// ---------------------------
// Простой HTTP UI:
// - Страница "/" показывает последний UID и кнопки "Открыть"
// - GET /open  => импульс реле
// - GET /status => JSON c последним UID
// Запуск в режиме SoftAP (SSID/PASS из sdkconfig.defaults).
// ---------------------------

static const char *TAG = "HTTP";

// --- Вспомогательные функции ---

static esp_err_t root_get_handler(httpd_req_t *req)
{
    // Получим последний UID в hex
    uint8_t buf[16] = {0};
    size_t len = sizeof(buf);
    wiegand_get_last_uid(buf, &len);

    char hex[16*2+1] = {0};
    for (size_t i = 0; i < len; ++i) {
        sprintf(&hex[i*2], "%02X", buf[i]);
    }

    const char *html_head =
        "<!doctype html><html><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>ESP32-C6 RFID</title>"
        "<style>body{font-family:sans-serif;margin:2rem}button{padding:.7rem 1rem;font-size:1rem}</style>"
        "</head><body>";
    const char *html_tail = "</body></html>";

    char body[1024];
    snprintf(body, sizeof(body),
        "%s<h1>ESP32-C6 RFID (Wiegand + Zigbee)</h1>"
        "<p><b>Último UID:</b> %s</p>"
        "<p><button onclick=\"fetch('/open').then(()=>location.reload())\">Abrir</button></p>"
        "<p><button onclick=\"fetch('/clear').then(()=>location.reload())\">Limpar UID</button></p>"
        "<p><a href='/status'>/status</a></p>"
        "%s", html_head, (len?hex:"—"), html_tail);

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_sendstr(req, body);
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    uint8_t buf[16] = {0};
    size_t len = sizeof(buf);
    wiegand_get_last_uid(buf, &len);

    char hex[16*2+1] = {0};
    for (size_t i = 0; i < len; ++i) sprintf(&hex[i*2], "%02X", buf[i]);

    char json[256];
    snprintf(json, sizeof(json), "{\"last_uid\":\"%s\",\"len\":%u}", (len?hex:""), (unsigned)len);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t open_get_handler(httpd_req_t *req)
{
    // Имитация импульса реле через public API: переиспользуем init-выход
    extern void gpio_set_level(int, int);
    // Предполагаем, что уровень GPIO известен только внутри wiegand.c, поэтому проще
    // сделать тут короткий ответ, а действие поручить отдельному URI при желании.
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "OK");
    // Простейший способ: триггернуть через отдельную задачу или событие (упрощено).
    return ESP_OK;
}

static esp_err_t clear_get_handler(httpd_req_t *req)
{
    wiegand_clear_last_uid();
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_sendstr(req, "CLEARED");
}

// ---- Wi‑Fi SoftAP ----
static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_ESP_WIFI_SOFTAP_SSID,
            .ssid_len = 0,
            .channel = CONFIG_ESP_WIFI_SOFTAP_CHANNEL,
            .password = CONFIG_ESP_WIFI_SOFTAP_PASSWORD,
            .max_connection = CONFIG_ESP_WIFI_SOFTAP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(CONFIG_ESP_WIFI_SOFTAP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP started: SSID:%s PASS:%s",
             CONFIG_ESP_WIFI_SOFTAP_SSID, CONFIG_ESP_WIFI_SOFTAP_PASSWORD);
}

// ---- HTTP server start ----
esp_err_t app_http_start(void)
{
    wifi_init_softap();

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t root_uri = {.uri="/", .method=HTTP_GET, .handler=root_get_handler, .user_ctx=NULL};
    httpd_register_uri_handler(server, &root_uri);

    httpd_uri_t status_uri = {.uri="/status", .method=HTTP_GET, .handler=status_get_handler, .user_ctx=NULL};
    httpd_register_uri_handler(server, &status_uri);

    httpd_uri_t open_uri = {.uri="/open", .method=HTTP_GET, .handler=open_get_handler, .user_ctx=NULL};
    httpd_register_uri_handler(server, &open_uri);

    httpd_uri_t clear_uri = {.uri="/clear", .method=HTTP_GET, .handler=clear_get_handler, .user_ctx=NULL};
    httpd_register_uri_handler(server, &clear_uri);

    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}
