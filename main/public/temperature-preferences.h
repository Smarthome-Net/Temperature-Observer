#pragma once

#include <stdio_ext.h>
#include "esp_log.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_handle.hpp"
#include "string.h"

struct Temperature_preferences_t
{
    char *ssid;
    char *password;
    char *mqtt_host;
    uint32_t mqtt_port;
    uint32_t measure_intervall;
    char *room;
    char *name;
};

class Temperature_preferences
{
private:
    char *read_string(nvs::NVSHandle *handle, const char *key);
public:
    Temperature_preferences();
    ~Temperature_preferences();
    esp_err_t load_preferences(Temperature_preferences_t *preferences);
    esp_err_t save_prefrenecs(Temperature_preferences_t *preferences);

    esp_err_t set_factory_default(int force_factory = 0);
};
