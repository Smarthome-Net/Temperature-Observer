#pragma once

#include <cstdint>
#include "../nlohmann/json.hpp"

using json = nlohmann::json;
using namespace nlohmann::literals;

namespace models
{
    struct Temperature_preferences_t
    {
        std::string ssid;
        std::string password;
        std::string mqtt_host;
        uint32_t mqtt_port;
        uint32_t measure_interval;
        std::string room;
        std::string device_name;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Temperature_preferences_t, ssid, password, mqtt_host, mqtt_port, measure_interval, room, device_name);  
    };
}