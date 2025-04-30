#include "../nlohmann/json.hpp"

using json = nlohmann::json;
using namespace nlohmann::literals;

namespace models 
{
    struct Temperature_value_t
    {
        float value;
        int64_t time;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Temperature_value_t, value, time); 
    };
}