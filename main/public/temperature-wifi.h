#pragma once

#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "temperature-mqtt-client.h"

class Temperature_wifi
{

private:
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t *config;
  Temperature_mqtt_client *mqtt_client;
  EventGroupHandle_t temperature_wifi_event_group;
  bool is_connected = false;

  /**
   * Create the event loop to handle wifi event
   * @return an esp error code
   */
  esp_err_t create_event_loop();

  /**
   * Create a wifi access point
   * @return an esp error code
  */
  esp_err_t create_wifi_ap();

  /**
   * Create a wifi station, that can connect to an access point
   * @return esp error code
  */
  esp_err_t start_wifi_sta();

  /**
   * Log an esp error code, with a message
   * @param err_code the error code
   * @param message the message
  */
  void log_err_code(esp_err_t err_code, const char* message);

  /**
   * Call wifi connect and log error if not succeed
  */
  void connect();

  /**
   * Wait for the event group handle
  */
  void event_group_wait();

public:
  esp_netif_t *netif;
  /**
   * constructor
   * @param config wifi configuration
  */
  Temperature_wifi(wifi_config_t *config, Temperature_mqtt_client *mqtt_client);
  ~Temperature_wifi();

  /**
   * Start the wifi client
   * @return esp error code. ESP_OK means succeed
   */
  esp_err_t start_wifi();

  /**
   * Consume a wifi event
   * @param id event id for wifi events
   * @param event_data data from the event, should be cast to specific typ
  */
  void consume_wifi_event(int32_t id, void* event_data);

  /**
   * Consume an ip event
   * @param id event id for ip events
   * @param event_date data from the event, should be cast to specific typ
  */
  void consume_ip_event(int32_t id, void* event_data);

  bool get_is_connected();
};


