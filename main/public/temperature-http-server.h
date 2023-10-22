#pragma once

#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "temperature-preferences.h"

class Temperature_http_server
{
private:
    httpd_handle_t server = NULL;
    httpd_config_t config;

    void (*device_status)(cJSON *json);
    Temperature_preferences *preferences;
public:
    Temperature_http_server(void (*status)(cJSON *json), Temperature_preferences *preferences);
    ~Temperature_http_server();

    esp_err_t start_server();
    esp_err_t register_endpoints();

    esp_err_t get_index_handler(httpd_req_t *req);
    esp_err_t get_status_handler(httpd_req_t *req);
    esp_err_t update_config_handler(httpd_req_t *req);
};
