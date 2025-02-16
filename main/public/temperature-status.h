#pragma once

#include "models/temperature_device_status_t.h"

class Temperature_status
{
private:
    models::Temperature_device_status_t status {};
public:
    Temperature_status();

    void set_wifi_status(bool status);
    void set_mqtt_status(bool status);
    void set_last_temperature(float temperature);
    void set_battery_staus(float battery);
    models::Temperature_device_status_t get_device_status();
};

