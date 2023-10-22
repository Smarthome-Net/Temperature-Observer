#include "temperature-observer.h"

static const char *TAG = "temperature_observer";

static const int INTERVAL = 10;

Temperature_observer::Temperature_observer(Temperature_mqtt_client *mqtt_client)
{
  this->mqtt_client = mqtt_client;
  this->ds18b20_info = ds18b20_malloc();
  this->oneWireBus = owb_rmt_initialize(&this->rmt_driver_info, GPIO_NUM_25, RMT_CHANNEL_1, RMT_CHANNEL_0);
  ESP_LOGI(TAG, "ds18b20 created, and init is: %d", this->ds18b20_info->init);
}

Temperature_observer::~Temperature_observer()
{
  ds18b20_free(&this->ds18b20_info);
  owb_uninitialize(this->oneWireBus);
}

esp_err_t Temperature_observer::init_sensor() 
{
  // error and status codes
  owb_status owb_status;
  DS18B20_ERROR ds18b20_error;

  // set crc to true
  owb_status = owb_use_crc(this->oneWireBus, true);
  ESP_LOGI(TAG, "Use crc status: %d, Use crc: %d", owb_status, this->oneWireBus->use_crc);
  if(owb_status != OWB_STATUS_OK)
  {
    return ESP_FAIL;
  }

  ds18b20_init_solo(this->ds18b20_info, this->oneWireBus);
  ds18b20_use_crc(this->ds18b20_info, true);
  bool is_successful = ds18b20_set_resolution(this->ds18b20_info, DS18B20_RESOLUTION_12_BIT);
  ESP_LOGI(TAG,"Init ds18b20: %d", is_successful);

  // parasitic power default false
  bool withParasiticPower = false;
  ESP_LOGI(TAG, "Check and set parasite power");
  ds18b20_error = ds18b20_check_for_parasite_power(this->oneWireBus, &withParasiticPower);
  if(ds18b20_error != DS18B20_OK)
  {
    return ESP_FAIL;
  }

  owb_status = owb_use_parasitic_power(this->oneWireBus, withParasiticPower);
  if(owb_status != OWB_STATUS_OK)
  {
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Check succeded parasitic power is %d", withParasiticPower);
  this->is_sensor_init = true;
  return ESP_OK;
}

esp_err_t Temperature_observer::start()
{
  if(!this->is_sensor_init) {
    return ESP_FAIL;
  }

  bool is_running = true;
  
  struct timeval current_time;
  struct tm execution_time = this->calculate_measure_time(current_time);

  ESP_LOGI(TAG, "The next measure time is: %s", asctime(&execution_time));
  while(is_running) 
  {
    struct tm* current_tm;
    gettimeofday(&current_time, NULL);
    current_tm = localtime(&current_time.tv_sec);
    
    if(current_tm->tm_min == execution_time.tm_min) {
      ESP_ERROR_CHECK(this->measure());

      execution_time = this->calculate_measure_time(current_time);
      ESP_LOGI(TAG, "The next measure time is: %s", asctime(&execution_time));
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  return ESP_OK;
}

esp_err_t Temperature_observer::measure() 
{
  float temperature_value;
  DS18B20_ERROR result_state = this->read_temperature(&temperature_value);
  if(result_state != DS18B20_OK) {
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Current temperature is: %f", temperature_value);
  this->mqtt_client->publish_message(temperature_value);
  return ESP_OK;
}

DS18B20_ERROR Temperature_observer::read_temperature(float *temperature_value)
{
  ds18b20_convert_all(this->oneWireBus);
  ds18b20_wait_for_conversion(this->ds18b20_info);
  return ds18b20_read_temp(this->ds18b20_info, temperature_value);
}

esp_err_t Temperature_observer::get_sensor_status(Temperature_observer_status_t *status)
{
  status->is_sensor_init = is_sensor_init;
  if(!this->is_sensor_init) {
    return ESP_OK;
  }

  float temperature_value;
  DS18B20_ERROR result_state = this->read_temperature(&temperature_value);
  if(result_state != DS18B20_OK) {
    return ESP_FAIL;
  }

  status->current_temperature = temperature_value;
  return ESP_OK;
}

tm Temperature_observer::calculate_measure_time(timeval current_time)
{
  struct tm execution_time;

  gettimeofday(&current_time, NULL);
  execution_time = *localtime(&current_time.tv_sec);

  int moduloResult = execution_time.tm_min % INTERVAL;
  int minute = execution_time.tm_min - moduloResult + INTERVAL;
  execution_time.tm_min = minute;
  execution_time.tm_sec = 0;

  //if reach 60 we swap the hour and reset the min to 0
  if(minute == 60) {
    execution_time.tm_hour = execution_time.tm_hour + 1;
    execution_time.tm_min = 0;
  }

  //if we reach midnight...
  if(execution_time.tm_hour == 24) {
    execution_time.tm_hour = 0;
  }

  return execution_time;
}


