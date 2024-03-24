#include "temperature-mqtt-client.h"

static const char *TAG = "temperature_mqtt_client";
const char *BASE_TOPIC = "smarthome/sensors";

#define MQTT_CONNECTED_BIT BIT0
#define MQTT_FAIL_BIT BIT1

static int s_retry_count = 0;

#define MQTT_MAXIUM_RETRY CONFIG_MAXIMUN_CONNECT_RETRY

#define ROOM CONFIG_ROOM

#define NAME CONFIG_NAME

// workaround to go back to observer object scope, so we can set the connected bit and call other object stuff
// the observer was set in the constructor, the event handler was set when the mqtt client start to connect

static void mqtt_event_handler_static(void *event_handler_arg, esp_event_base_t event_base, int32_t id, void *event_data)
{
  Temperature_mqtt_client *client = (Temperature_mqtt_client *)event_handler_arg;
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  if(client != NULL)
  {
    client->consume_mqtt_event(id);
  }
}


Temperature_mqtt_client::Temperature_mqtt_client(esp_mqtt_client_config_t* mqtt_config)
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
  this->mqtt_client = esp_mqtt_client_init(this->mqtt_config);
  if(this->mqtt_client == NULL)
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
  if(return_code == ESP_OK)
  {
    EventBits_t bits = xEventGroupWaitBits(this->mqtt_event_group, MQTT_CONNECTED_BIT | MQTT_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if(bits & MQTT_CONNECTED_BIT)
    {
      ESP_LOGI(TAG, "Connected to MQTT Broker");
      return ESP_OK;
    }
    else if(bits & MQTT_FAIL_BIT)
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
  if(!this->is_connected) {
    return ESP_ERR_INVALID_STATE;
  }
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  int64_t seconds = (int64_t)current_time.tv_sec * 1000L;
  
  const char* topic = this->get_topic();
  nlohmann::json json = value;
  const char *sJson = json.dump().c_str();
  ESP_LOGI(TAG, "%s", sJson);

  int status = esp_mqtt_client_publish(this->mqtt_client, topic, sJson, strlen(sJson), 0, 0);
  if(status == -1) {
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t Temperature_mqtt_client::consume_mqtt_event(int32_t event)
{
  switch (event)
  {
    case MQTT_EVENT_ERROR:
      ESP_LOGI(TAG, "ESP Erro event");
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
      if(s_retry_count < MQTT_MAXIUM_RETRY)
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

const char* Temperature_mqtt_client::get_topic() {
  const char *temperature = "temperature";
  const char* room = (char*) ROOM;
  const char* name = (char*) NAME;

  size_t size = strlen(BASE_TOPIC);
  size += strlen("/");
  size *= strlen(temperature);
  size += strlen("/");
  size += strlen(room);
  size += strlen("/");
  size += strlen(name);
  size += 1;

  char* buffer = (char *)malloc(size);
  snprintf(buffer, size, "%s/%s/%s/%s", BASE_TOPIC, temperature, room, name);
  ESP_LOGD(TAG, "Fulltopic: %s", buffer);

  return buffer;
}

const char* Temperature_mqtt_client::get_rpc_subscribe_topic(const char* endpoint) 
{
  const char *RPC = "RPC";
  const char* room = (char*) ROOM;
  const char* name = (char*) NAME;

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

  char* buffer = (char *)malloc(size);
  snprintf(buffer, size, "%s.%s/%s/%s/%s", BASE_TOPIC, RPC, room, name, endpoint);
  ESP_LOGD(TAG, "Fulltopic: %s", buffer);

  return buffer;
}

const char* Temperature_mqtt_client::get_rpc_response_topic(const char* topic) 
{
  const char *response = "response";

  size_t size = strlen(topic);
  size += strlen("/");
  size *= strlen(response);
  size += 1;

  char* buffer = (char *)malloc(size);
  snprintf(buffer, size, "%s/%s", topic, response);
  ESP_LOGD(TAG, "Fulltopic: %s", buffer);

  return buffer;
}