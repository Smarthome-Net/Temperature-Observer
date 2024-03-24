#include "../nlohmann/json.hpp"

using json = nlohmann::json;
using namespace nlohmann::literals;

namespace models 
{
    struct Temperature_value_t
    {
        float value;
        int64_t time;

        friend void to_json(nlohmann::json& json, const Temperature_value_t& value) 
        {
            json["value"] = value.value;
            json["time"] = value.time;
        }
        
        friend void from_json(const nlohmann::json& json, Temperature_value_t& value)
        {
            json.at("value").get_to(value.value);
            json.at("time").get_to(value.time);
        }  
    };
}