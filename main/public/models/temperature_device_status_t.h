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

        friend void to_json(nlohmann::json& json, const Temperature_device_status_t& value) 
        {
            json["wifiConnectedStatus"] = value.wifi_connected_status;
            json["mqttConnectedStatus"] = value.wifi_connected_status;
            json["batteryStatus"] = value.battery_status;
            json["currentTemperature"] = value.current_temperature;
        }
        
        friend void from_json(const nlohmann::json& json, Temperature_device_status_t& value)
        {
            json.at("wifiConnectedStatus").get_to(value.wifi_connected_status);
            json.at("mqttConnectedStatus").get_to(value.wifi_connected_status);
            json.at("batteryStatus").get_to(value.battery_status);
            json.at("currentTemperature").get_to(value.current_temperature);
        } 
    };
}