#pragma once

#include "esp_log.h"
#include "ds18b20.h"
#include "owb.h"

class Temperature_observer
{
private:
  owb_rmt_driver_info rmt_driver_info;
  OneWireBus *oneWireBus;
  DS18B20_Info *ds18b20_info;
public:
  Temperature_observer();
  ~Temperature_observer();

  esp_err_t init_sensor();

  DS18B20_ERROR read_temperature(float *temperature_value);
};


