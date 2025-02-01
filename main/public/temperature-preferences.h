#pragma once

#include <stdio_ext.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_handle.hpp"
#include "string.h"
#include "models/temperature_preferences_t.h"
#include "models/temperature_mqtt_config_t.h"


class Temperature_preferences
{
private:
    char *read_string(nvs::NVSHandle *handle, const char *key);
    uint32_t read_uint32_t(nvs::NVSHandle *handle, const char *key);
public:
    Temperature_preferences();
    ~Temperature_preferences();
    esp_err_t load_preferences(models::Temperature_preferences_t *preferences);
    esp_err_t save_prefrenecs(models::Temperature_preferences_t *preferences);

    esp_err_t load_wifi_config(wifi_config_t *wifi_config);
    esp_err_t load_mqtt_config(models::Temperature_mqtt_config_t *mqtt_config);

    esp_err_t set_factory_default(int force_factory = 0);
};
