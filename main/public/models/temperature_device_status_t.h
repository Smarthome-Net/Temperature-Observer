#pragma once

#include "../nlohmann/json.hpp"

using json = nlohmann::json;
using namespace nlohmann::literals;

namespace models 
{
    struct Temperature_device_status_t 
    {
        bool is_wifi_connected;
        bool is_mqtt_connected;
        float battery_status;
        float current_temperature;

        friend void to_json(nlohmann::json& json, const Temperature_device_status_t& value) 
        {
            json["isWifiConnected"] = value.is_wifi_connected;
            json["isMqttConnected"] = value.is_wifi_connected;
            json["batteryStatus"] = value.battery_status;
            json["currentTemperature"] = value.current_temperature;
        }
        
        friend void from_json(const nlohmann::json& json, Temperature_device_status_t& value)
        {
            json.at("isWifiConnected").get_to(value.is_wifi_connected);
            json.at("isMqttConnected").get_to(value.is_mqtt_connected);
            json.at("batteryStatus").get_to(value.battery_status);
            json.at("currentTemperature").get_to(value.current_temperature);
        } 
    };
}