#pragma once

#include <cstdint>
#include "../nlohmann/json.hpp"

using json = nlohmann::json;
using namespace nlohmann::literals;

namespace models
{
    struct Temperature_preferences_t
    {
        const char *ssid;
        const char *password;
        const char *mqtt_host;
        uint32_t mqtt_port;
        uint32_t measure_intervall;
        const char *room;
        const char *name;

        friend void to_json(nlohmann::json& json, const Temperature_preferences_t& value) 
        {
            json["ssid"] = value.ssid;
            json["password"] = value.password;
            json["mqttHost"] = value.mqtt_host;
            json["mqttPort"] = value.mqtt_port;
            json["measureIntervall"] = value.measure_intervall;
            json["room"] = value.room;
            json["name"] = value.name;
        }
        
        friend void from_json(const nlohmann::json& json, Temperature_preferences_t& value)
        {
            value.ssid = json.at("ssid").get<std::string>().c_str();
            value.password = json.at("password").get<std::string>().c_str();
            value.mqtt_host = json.at("mqttHost").get<std::string>().c_str();
            json.at("mqttPort").get_to(value.mqtt_port);
            json.at("measureIntervall").get_to(value.measure_intervall);
            value.room = json.at("room").get<std::string>().c_str();
            value.name = json.at("name").get<std::string>().c_str();
        }    
    };
}