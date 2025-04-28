#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "temperature-observer.h"
#include "temperature-wifi.h"
#include "temperature-preferences.h"
#include "temperature-mqtt-client.h"
#include "temperature-led.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "time.h"
#include "models/temperature_preferences_t.h"

static const char *TAG = "temperature_main";

// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TIMEZONE "CET-1CEST,M3.5.0/02,M10.5.0/03"

#define WIFI_SSID CONFIG_WIFI_SSID

#define WIFI_PASSWORD CONFIG_WIFI_PASSWORD

#define BROKER_HOST CONFIG_BROKER_HOST

#define BROKER_PORT CONFIG_BROKER_PORT

#if CONFIG_TRANSPORT_MQTT
#define TRANSPORT MQTT_TRANSPORT_OVER_TCP
#endif

#if CONFIG_TRANSPORT_MQTTS
#define TRANSPORT MQTT_TRANSPORT_OVER_SSL
#endif

#if CONFIG_TRANSPORT_WS
#define TRANSPORT MQTT_TRANSPORT_OVER_WS
#endif

#if CONFIG_PROTOCOL_WSS
#define TRANSPORT MQTT_TRANSPORT_OVER_WSS
#endif

Temperature_observer *observer;
Temperature_mqtt_client *mqtt_client;
Temperature_wifi *wifi_client;
Temperature_status *status;
TaskHandle_t main_handle = NULL;

extern "C" {
  void app_main(void);
}

void sync_time_callback(struct timeval *tv) {
  struct tm *time;
  gettimeofday(tv, NULL);
  time = localtime(&tv->tv_sec);
  ESP_LOGI(TAG, "The current time is: %s", asctime(time));
  vTaskResume(main_handle);
}

tm calculate_measure_time(uint32_t intervall)
{
  time_t now;
  struct tm execution_time;

  time(&now);
  localtime_r(&now, &execution_time);

  int moduloResult = execution_time.tm_min % intervall;
  int minute = execution_time.tm_min - moduloResult + intervall;
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

  ESP_LOGI(TAG, "First measuring at: %s", asctime(&execution_time));
  return execution_time;
}

void app_main()
{
  ESP_LOGI(TAG, "Start Temperature Observer");

  ESP_LOGI(TAG, "Initialize Non Volatile Storage");
  esp_err_t err_code = nvs_flash_init();
  Temperature_preferences* preference = new Temperature_preferences();
  if(err_code == ESP_ERR_NVS_NO_FREE_PAGES || err_code == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGI(TAG, "Erase nvs and initialize again");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err_code = nvs_flash_init();
    preference->set_factory_default();
  }

  ESP_ERROR_CHECK(err_code);
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  main_handle = xTaskGetCurrentTaskHandle();
  setenv("TZ", TIMEZONE, 1);
  tzset(); 
  
  // workaround initialization for wifi_config to avoid outside aggregate initializer in c++
  wifi_config_t wifi_config = { };
  status = new Temperature_status();
  preference->load_wifi_config(&wifi_config);
  wifi_client = new Temperature_wifi(&wifi_config, status);
  ESP_ERROR_CHECK(wifi_client->start_wifi());

  models::Temperature_mqtt_config_t mqtt_config = { };
  preference->load_mqtt_config(&mqtt_config);
  mqtt_client = new Temperature_mqtt_client(&mqtt_config, status);
  observer = new Temperature_observer();
  observer->init_sensor();
  ESP_ERROR_CHECK(mqtt_client->connect_mqtt());
  ESP_ERROR_CHECK(mqtt_client->subscribe_status());
  ESP_ERROR_CHECK(mqtt_client->subscribe_settings());

  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_set_sync_interval(24 * 60 * 60 * 1000);
  esp_sntp_set_time_sync_notification_cb(&sync_time_callback);
  esp_sntp_init();
  vTaskSuspend(main_handle);
  
  uint32_t intervall = 0;
  ESP_ERROR_CHECK(preference->load_intervall(&intervall));
  ESP_LOGI(TAG, "Intervall in minutes: %lu", intervall);
  time_t now;
  struct tm executing_time = calculate_measure_time(intervall);
  time(&now);
  const double diffTime = difftime(mktime(&executing_time), now);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xIntial = pdMS_TO_TICKS(diffTime * 1000);
  xTaskDelayUntil(&xLastWakeTime, xIntial);
  
  const TickType_t xFrequency = pdMS_TO_TICKS(intervall * 60 * 1000);
  xLastWakeTime = xTaskGetTickCount();
  while (true)
  {
    float value;
    DS18B20_ERROR err = observer->read_temperature(&value);
    if(err == DS18B20_OK) 
    {
      status->set_last_temperature(value);
      struct tm *measure_time_tm;
      struct timeval measure_time_tv;
      gettimeofday(&measure_time_tv, NULL);
      measure_time_tm = localtime(&measure_time_tv.tv_sec);
      int64_t seconds = (int64_t)measure_time_tv.tv_sec * 1000L;
      ESP_LOGI(TAG, "Measuring time is: %s", asctime(measure_time_tm));
      models::Temperature_value_t temperature_value = { 
        .value = value,
        .time = seconds
      };
      ESP_ERROR_CHECK(mqtt_client->publish_temperature_value(temperature_value));
      xTaskDelayUntil(&xLastWakeTime, xFrequency);
      xLastWakeTime = xTaskGetTickCount();
    }
  }

  printf("End of Application \n");
  fflush(stdout);
}
