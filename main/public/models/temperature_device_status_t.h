#pragma once

#include "../nlohmann/json.hpp"

using json = nlohmann::json;
using namespace nlohmann::literals;

namespace models 
{
    enum Connection_status_t 
    {
        Unknown = -1,
        Disconnected = 0,
        Connected = 1,
    };
    
    struct Temperature_device_status_t 
    {
        Connection_status_t wifi_connected_status;
        Connection_status_t mqtt_connected_status;
        float battery_status;
        float current_temperature;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Temperature_device_status_t, wifi_connected_status, mqtt_connected_status, battery_status, current_temperature);
    };
}