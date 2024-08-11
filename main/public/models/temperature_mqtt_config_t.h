#pragma once

#include "mqtt_client.h"


namespace models
{
    struct Temperature_mqtt_config_t
    {
        const char* room;
        const char* name;
        esp_mqtt_client_config_t* mqtt_config;
    };

}