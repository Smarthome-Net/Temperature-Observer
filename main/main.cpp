/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "temperature-observer.h"
#include "temperature-wifi.h"
#include "temperature-http-server.h"
#include "temperature-preferences.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "time.h"
#include "models/temperate_preferences_t.h"

static const char *TAG = "temperature_main";

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

extern "C" {
  void app_main(void);
}

void start_sync_time() 
{
  bool isTimeNotSync = true;
  time_t now;
  char time_buf[64];
  struct tm info;
  const TickType_t xDelay = 5000 / portTICK_PERIOD_MS;

  do
  {
    ESP_LOGI(TAG, "Waiting for time sync...");
    vTaskDelay(xDelay);
    
    sntp_sync_status_t sync_status = sntp_get_sync_status();
    if(sync_status == SNTP_SYNC_STATUS_COMPLETED) 
    {
      ESP_LOGI(TAG, "Time is syncronized now");
      isTimeNotSync = false;
    }
  } while (isTimeNotSync);
  
  time(&now);
  localtime_r(&now, &info);
  strftime(time_buf, sizeof(time_buf), "%c", &info);
  ESP_LOGI(TAG, "The current time is: %s", time_buf);
}

void get_device_status(cJSON *json) 
{
  cJSON_AddNumberToObject(json, "CurrentTemperature", 42);
  cJSON_AddNumberToObject(json, "BatteryStatus", 42);
  cJSON_AddBoolToObject(json, "IsWifiConnected", false);
  cJSON_AddBoolToObject(json, "IsMqttConnected", false);
}

void app_main()
{
  ESP_LOGI(TAG, "Start Temperature Observer");

  ESP_LOGI(TAG, "Initialize Non Volatile Storage");
  esp_err_t err_code = nvs_flash_init();
  if(err_code == ESP_ERR_NVS_NO_FREE_PAGES || err_code == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGI(TAG, "Erase nvs and initialize again");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err_code = nvs_flash_init();
  }

  ESP_ERROR_CHECK(err_code);
  esp_netif_init();

  Temperature_preferences* preference = new Temperature_preferences();
  models::Temperature_preferences_t data;
  //preference->set_factory_default();
  preference->load_preferences(&data);
  ESP_LOGI(TAG, "Mqtt Host: %s", data.mqtt_host);
  ESP_LOGI(TAG, "Mqtt Port: %li", data.mqtt_port);
  ESP_LOGI(TAG, "Interval: %li", data.measure_intervall);
  ESP_LOGI(TAG, "WIFI SSID: %s", data.ssid);
  ESP_LOGI(TAG, "Room: %s", data.room);
  ESP_LOGI(TAG, "Name: %s", data.name);
  
  setenv("TZ", "CEST-1CET", 1);
  tzset();
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_init();
  
  // workaround initialization for wifi_config to avoid outside aggregate initializer in c++
  wifi_config_t wifi_config = { };
  strcpy((char*)wifi_config.sta.ssid, data.ssid);
  strcpy((char*)wifi_config.sta.password, data.password);
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;

  esp_mqtt_client_config_t mqtt_config = { };
  mqtt_config.broker.address.hostname = data.mqtt_host;
  mqtt_config.broker.address.port = data.mqtt_port;
  mqtt_config.broker.address.transport = TRANSPORT;

  Temperature_mqtt_client* mqtt_client = new Temperature_mqtt_client(&mqtt_config);

  Temperature_wifi* wifi_client = new Temperature_wifi(&wifi_config, mqtt_client);
  ESP_ERROR_CHECK(wifi_client->start_wifi());
  start_sync_time();

  Temperature_http_server* server = new Temperature_http_server(&get_device_status, preference);
  ESP_ERROR_CHECK(server->start_server());
  ESP_ERROR_CHECK(server->register_endpoints());
  
  Temperature_observer* observer = new Temperature_observer(mqtt_client);
  ESP_ERROR_CHECK(observer->init_sensor());
  ESP_ERROR_CHECK(observer->start());
  
  printf("End of Application \n");
  fflush(stdout);
}


