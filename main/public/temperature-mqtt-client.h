#pragma once

#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <nlohmann/json.hpp>
#include "models/temperature_device_status_t.h"
#include "models/temperature_value_t.h"
#include "models/temperature_preferences_t.h"
#include "models/temperature_mqtt_config_t.h"


class Temperature_mqtt_client
{
  private:
    bool is_connected = false;
    esp_mqtt_client_handle_t mqtt_client;
    models::Temperature_mqtt_config_t* mqtt_config;
    EventGroupHandle_t mqtt_event_group;
    const char* get_topic();
    const char* get_rpc_subscribe_topic(const char* endpoint);
    const char* get_rpc_response_topic(const char* topic);
    int ends_with(const char* str, const char* suffix);
    esp_err_t publish(const char* topic, const char* data);
  
  public:
    Temperature_mqtt_client(models::Temperature_mqtt_config_t* mqtt_config);
    ~Temperature_mqtt_client();
    esp_err_t consume_mqtt_event(int32_t event, void *event_data);
    
    esp_err_t publish_temperature_value(models::Temperature_value_t value);
    esp_err_t publish_status(models::Temperature_device_status_t status, const char* topic);
    esp_err_t publish_settings(models::Temperature_preferences_t settings, const char *topic);
    

    esp_err_t subscribe_status();
    esp_err_t subscribe_settings();
    
    esp_err_t connect_mqtt();
    bool get_is_connected();

    void handle_mqtt_data(esp_mqtt_event_handle_t data);
};


