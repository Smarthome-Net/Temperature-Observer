#include "temperature-mqtt-client.h"

static const char *TAG = "temperature_mqtt_client";
const char *BASE_TOPIC = "smarthome/sensors";

#define MQTT_CONNECTED_BIT BIT0
#define MQTT_FAIL_BIT BIT1

static int s_retry_count = 0;

#define MQTT_MAXIUM_RETRY CONFIG_MAXIMUN_CONNECT_RETRY

// workaround to go back to observer object scope, so we can set the connected bit and call other object stuff
// the observer was set in the constructor, the event handler was set when the mqtt client start to connect
static void mqtt_event_handler_static(void *event_handler_arg, esp_event_base_t event_base, int32_t id, void *event_data)
{
  Temperature_mqtt_client *client = (Temperature_mqtt_client *)event_handler_arg;
  if (client != NULL)
  {
    client->consume_mqtt_event(id, event_data);
  }
}

Temperature_mqtt_client::Temperature_mqtt_client(models::Temperature_mqtt_config_t *mqtt_config)
{
  this->mqtt_config = mqtt_config;
}

Temperature_mqtt_client::~Temperature_mqtt_client()
{
}

esp_err_t Temperature_mqtt_client::connect_mqtt()
{
  ESP_LOGI(TAG, "Connect to MQTT");
  this->mqtt_event_group = xEventGroupCreate();
  this->mqtt_client = esp_mqtt_client_init(&this->mqtt_config->mqtt_config);
  if (this->mqtt_client == NULL)
  {
    ESP_LOGW(TAG, "Unable to create mqtt client");
    return ESP_FAIL;
  }
  esp_mqtt_client_register_event(this->mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler_static, this);
  ESP_LOGI(TAG, "Mqtt client created, start now");
  esp_err_t return_code = esp_mqtt_client_start(this->mqtt_client);
  const char *name = esp_err_to_name(return_code);
  ESP_LOGI(TAG, "Return code for mqtt clients start: %s", name);
  ESP_ERROR_CHECK(return_code);
  if (return_code == ESP_OK)
  {
    EventBits_t bits = xEventGroupWaitBits(this->mqtt_event_group, MQTT_CONNECTED_BIT | MQTT_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & MQTT_CONNECTED_BIT)
    {
      ESP_LOGI(TAG, "Connected to MQTT Broker");
      return ESP_OK;
    }
    else if (bits & MQTT_FAIL_BIT)
    {
      ESP_LOGI(TAG, "Fail connect to MQTT Broker");
      esp_mqtt_client_stop(this->mqtt_client);
      return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Something unexpected happened");
    return ESP_FAIL;
  }
  return return_code;
}

esp_err_t Temperature_mqtt_client::publish_temperature_value(models::Temperature_value_t value)
{
  if (!this->is_connected)
  {
    return ESP_ERR_INVALID_STATE;
  }

  const char *topic = this->get_topic();
  nlohmann::json json = value;
  const char *sJson = json.dump().c_str();
  ESP_LOGI(TAG, "%s", sJson);

  int publish_status = esp_mqtt_client_publish(this->mqtt_client, topic, sJson, strlen(sJson), 0, 0);
  if (publish_status == -1)
  {
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t Temperature_mqtt_client::publish_status(models::Temperature_device_status_t device_status, const char *topic)
{
  nlohmann::json json = device_status;
  const char *sJson = json.dump().c_str();
  return this->publish(topic, sJson);
}

esp_err_t Temperature_mqtt_client::publish_settings(models::Temperature_preferences_t settings, const char *topic)
{
  nlohmann::json json = settings;
  const char *sJson = json.dump().c_str();
  return this->publish(topic, sJson);
}

esp_err_t Temperature_mqtt_client::subscribe_status()
{
  const char *topic = this->get_rpc_subscribe_topic("status");
  ESP_LOGI(TAG, "Subscribe to topic: %s", topic);
  esp_mqtt_client_subscribe(this->mqtt_client, topic, 0);
  return ESP_OK;
}

esp_err_t Temperature_mqtt_client::subscribe_settings()
{
  const char *topic = this->get_rpc_subscribe_topic("config");
  ESP_LOGI(TAG, "Subscribe to topic: %s", topic);
  esp_mqtt_client_subscribe(this->mqtt_client, topic, 0);
  return ESP_OK;
}

esp_err_t Temperature_mqtt_client::consume_mqtt_event(int32_t event, void *event_data)
{
  esp_mqtt_event_handle_t mqtt_event_data = (esp_mqtt_event_handle_t)event_data;
  switch (event)
  {
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "ESP Error event");
    xEventGroupSetBits(this->mqtt_event_group, MQTT_FAIL_BIT);
    break;
  case MQTT_EVENT_CONNECTED:
  {
    ESP_LOGI(TAG, "Connected to broker");
    this->is_connected = true;
    xEventGroupSetBits(this->mqtt_event_group, MQTT_CONNECTED_BIT);
    break;
  }
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "Message published");
    break;
  case MQTT_EVENT_DISCONNECTED:
    this->is_connected = false;
    if (s_retry_count < MQTT_MAXIUM_RETRY)
    {
      s_retry_count++;
      ESP_LOGI(TAG, "Retry to connect");
    }
    else
    {
      xEventGroupSetBits(this->mqtt_event_group, MQTT_FAIL_BIT);
    }
    ESP_LOGI(TAG, "Unable to connect mqtt broker");
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "Subscribe to broker");
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "Esp data event");
    this->handle_mqtt_data(mqtt_event_data);
    break;
  case MQTT_EVENT_BEFORE_CONNECT:
    ESP_LOGI(TAG, "Mqtt before connect");
    break;
  default:
    ESP_LOGI(TAG, "Unkown Event id: %li", event);
    break;
  }
  return ESP_OK;
}

bool Temperature_mqtt_client::get_is_connected()
{
  return is_connected;
}

void Temperature_mqtt_client::handle_mqtt_data(esp_mqtt_event_handle_t data)
{
  char *read_topic = (char *)malloc(data->topic_len + 1);
  strncpy(read_topic, data->topic, data->topic_len);
  read_topic[data->topic_len] = '\0';
  ESP_LOGI(TAG, "Received message on topic: %s", read_topic);
  if (strstr(read_topic, ".RPC") != NULL && ends_with(read_topic, "status"))
  {
    //status shouldn't contain data, so we ignore it
    models::Temperature_device_status_t status = {};
    status.battery_status = 100;
    status.current_temperature = 20.0;
    status.is_mqtt_connected = true;
    status.is_wifi_connected = true;
    this->publish_status(status, read_topic);
    
  }

  if (strstr(read_topic, ".RPC") != NULL && ends_with(read_topic, "config"))
  {
    //config can contain data:
    //with data: update settings and return the new settings
    //without data: return only the current settings
    if(data->data_len == 0)
    {
      ESP_LOGI(TAG, "No data received");
      return;
    }
    models::Temperature_preferences_t settings = {};
    settings.measure_intervall = 15;
    settings.mqtt_host = "mqtt_host";	
    settings.mqtt_port = 1883;
    settings.name = "name";
    settings.room = "room";
    settings.ssid = "ssid";
    settings.password = "password";
    this->publish_settings(settings, read_topic);
    
  }
}

const char *Temperature_mqtt_client::get_topic()
{
  const char *temperature = "temperature";
  const char *room = this->mqtt_config->room;
  const char *name = this->mqtt_config->name;

  size_t size = strlen(BASE_TOPIC);
  size += strlen("/");
  size *= strlen(temperature);
  size += strlen("/");
  size += strlen(room);
  size += strlen("/");
  size += strlen(name);
  size += 1;

  char *buffer = (char *)malloc(size);
  snprintf(buffer, size, "%s/%s/%s/%s", BASE_TOPIC, temperature, room, name);
  ESP_LOGD(TAG, "Fulltopic: %s", buffer);

  return buffer;
}

const char *Temperature_mqtt_client::get_rpc_subscribe_topic(const char *endpoint)
{
  const char *RPC = "RPC";
  const char *room = this->mqtt_config->room;
  const char *name = this->mqtt_config->name;

  size_t size = strlen(BASE_TOPIC);
  size += strlen(".");
  size += strlen(RPC);
  size += strlen("/");
  size += strlen(room);
  size += strlen("/");
  size += strlen(name);
  size += strlen("/");
  size += strlen(endpoint);
  size += 1;

  char *buffer = (char *)malloc(size);
  snprintf(buffer, size, "%s.%s/%s/%s/%s", BASE_TOPIC, RPC, room, name, endpoint);
  ESP_LOGD(TAG, "Fulltopic: %s", buffer);

  return buffer;
}

const char *Temperature_mqtt_client::get_rpc_response_topic(const char *topic)
{
  const char *response = "response";

  size_t size = strlen(topic);
  size += strlen("/");
  size *= strlen(response);
  size += 1;

  char *buffer = (char *)malloc(size);
  snprintf(buffer, size, "%s/%s", topic, response);
  ESP_LOGD(TAG, "Fulltopic: %s", buffer);

  return buffer;
}

int Temperature_mqtt_client::ends_with(const char *str, const char *suffix)
{
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  return (str_len >= suffix_len) &&
         (!memcmp(str + str_len - suffix_len, suffix, suffix_len));
}

esp_err_t Temperature_mqtt_client::publish(const char *topic, const char *data)
{
  const char *response_topic = this->get_rpc_response_topic(topic);

  int publish_status = esp_mqtt_client_publish(this->mqtt_client, response_topic, data, strlen(data), 0, 0);
  if (publish_status == -1)
  {
    return ESP_FAIL;
  }
  return ESP_OK;
}
