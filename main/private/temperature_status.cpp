#include "temperature-status.h"

Temperature_status::Temperature_status()
{
}

void Temperature_status::set_wifi_status(bool status)
{
    this->status.is_wifi_connected = status;
}

void Temperature_status::set_mqtt_status(bool status)
{
    this->status.is_mqtt_connected = status;
}

void Temperature_status::set_last_temperature(float temperature)
{
    this->status.current_temperature = temperature;
}

void Temperature_status::set_battery_staus(float battery)
{
    this->status.battery_status = battery;
}

models::Temperature_device_status_t Temperature_status::get_device_status()
{
    return this->status;
}
