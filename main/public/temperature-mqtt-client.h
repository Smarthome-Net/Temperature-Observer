#pragma once

#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include "sys/time.h"


class Temperature_mqtt_client
{
  private:
    bool is_connected = false;
    esp_mqtt_client_handle_t mqtt_client;
    esp_mqtt_client_config_t *mqtt_config;
    EventGroupHandle_t mqtt_event_group;
    const char* get_topic();

  public:
    Temperature_mqtt_client(esp_mqtt_client_config_t* mqtt_config);
    ~Temperature_mqtt_client();

    esp_err_t publish_message(float value);

    void connect_mqtt();

    esp_err_t consume_mqtt_event(int32_t event);

    bool get_is_connected();
};


