/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
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

extern "C" {
  void app_main(void);
}

void sync_time_callback(struct timeval *tv) {
  struct tm *time;
  gettimeofday(tv, NULL);
  time = localtime(&tv->tv_sec);
  ESP_LOGI(TAG, "The current time is: %s", asctime(time));
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

  Temperature_preferences* preference = new Temperature_preferences();
  models::Temperature_preferences_t data;
  preference->load_preferences(&data);
  
  setenv("TZ", TIMEZONE, 1);
  tzset(); 
  
  // workaround initialization for wifi_config to avoid outside aggregate initializer in c++
  wifi_config_t wifi_config = { };
  preference->load_wifi_config(&wifi_config);
  Temperature_wifi* wifi_client = new Temperature_wifi(&wifi_config);
  ESP_ERROR_CHECK(wifi_client->start_wifi());

  models::Temperature_mqtt_config_t mqtt_config = { };
  preference->load_mqtt_config(&mqtt_config);

  Temperature_mqtt_client* mqtt_client = new Temperature_mqtt_client(&mqtt_config);

  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_set_sync_interval(24 * 60 * 60 * 1000);
  esp_sntp_set_time_sync_notification_cb(&sync_time_callback);
  esp_sntp_init();

  printf("End of Application \n");
  fflush(stdout);
}


