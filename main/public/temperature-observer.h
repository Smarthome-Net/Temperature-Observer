#pragma once

#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "ds18b20.h"
#include "owb.h"
#include "sys/time.h"
#include "temperature-mqtt-client.h"

struct Temperature_observer_status_t {
  bool is_sensor_init;
  float current_temperature;
};

class Temperature_observer
{
private:
  owb_rmt_driver_info rmt_driver_info;
  OneWireBus *oneWireBus;
  DS18B20_Info *ds18b20_info;
  Temperature_mqtt_client *mqtt_client;

  bool is_sensor_init = false;

  tm calculate_measure_time(timeval current_time);
public:
  Temperature_observer(Temperature_mqtt_client* mqtt_client);
  ~Temperature_observer();

  esp_err_t start();

  esp_err_t init_sensor();

  esp_err_t measure();

  DS18B20_ERROR read_temperature(float *temperature_value);

  esp_err_t get_sensor_status(Temperature_observer_status_t *status);
};


