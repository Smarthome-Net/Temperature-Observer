#include "temperature-wifi.h"

static const char *TAG = "temperature_wifi";

static int s_retry_count = 0;

#define WIFI_MAXIUM_RETRY CONFIG_MAXIMUN_CONNECT_RETRY

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

void event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
  Temperature_wifi *temperature_wifi_ref = (Temperature_wifi *)handler_arg;
  // if the cast fail in any reasen or the handler_arg is null this not a valid wifi/ip event to consume
  if (temperature_wifi_ref == NULL || handler_arg == NULL)
  {
    ESP_LOGW(TAG, "Unexpected Event, handler_arg was null or cast fail");
    return;
  }

  if (base == WIFI_EVENT)
  {
    ESP_LOGI(TAG, "Consume Wifi Event");
    temperature_wifi_ref->consume_wifi_event(id, event_data);
  }
  else if (base == IP_EVENT)
  {
    ESP_LOGI(TAG, "Consume IP Event");
    temperature_wifi_ref->consume_ip_event(id, event_data);
  }
  else
  {
    ESP_LOGW(TAG, "Unexpected base event: %s", base);
  }
}

Temperature_wifi::Temperature_wifi(wifi_config_t *config, Temperature_mqtt_client *mqtt_client)
{
  this->config = config;
  this->mqtt_client = mqtt_client;
  this->temperature_wifi_event_group = xEventGroupCreate();
}

Temperature_wifi::~Temperature_wifi()
{
}

esp_err_t Temperature_wifi::create_event_loop()
{
  ESP_LOGI(TAG, "Create Event Loop");
  esp_err_t return_code;

  return_code = esp_event_loop_create_default();
  // only continue if ESP_OK
  if (return_code == ESP_OK)
  {
    return_code = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, this);
  }

  if (return_code == ESP_OK)
  {
    return_code = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, this);

    if (return_code == ESP_OK)
    {
      ESP_LOGI(TAG, "Event Loop Succesfully created.");
    }
  }

  // if something went wrong
  if (return_code != ESP_OK)
  {
    this->log_err_code(return_code, "Fail to create and register event handler");
  }
  return return_code;
}

esp_err_t Temperature_wifi::create_wifi_ap()
{
  esp_err_t return_code;
  return_code = esp_wifi_set_mode(WIFI_MODE_AP);
  return_code = esp_wifi_set_config(WIFI_IF_AP, this->config);
  return return_code;
}

esp_err_t Temperature_wifi::start_wifi_sta()
{
  esp_err_t return_code;
  this->netif = esp_netif_create_default_wifi_sta();
  return_code = esp_wifi_set_mode(WIFI_MODE_STA);

  if (return_code == ESP_OK)
  {
    return_code = esp_wifi_set_config(WIFI_IF_STA, this->config);
  }

  if (return_code == ESP_OK)
  {
    return_code = esp_wifi_start();
  }

  if (return_code != ESP_OK)
  {
    this->log_err_code(return_code, "fail to start wifi station");
  }

  return return_code;
}

esp_err_t Temperature_wifi::start_wifi()
{
  ESP_LOGI(TAG, "Start Wifi");
  esp_err_t return_code;

  return_code = this->create_event_loop();
  if (return_code == ESP_OK)
  {
    esp_wifi_init(&this->init_config);
    return_code = this->start_wifi_sta();
    ESP_LOGI(TAG, "Station started");
  }
  if (return_code == ESP_OK)
  {
    this->event_group_wait();
  }
  return return_code;
}

void Temperature_wifi::consume_ip_event(int32_t id, void *event_data)
{
  switch (id)
  {
  case IP_EVENT_STA_GOT_IP:
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got ip: " IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_count = 0;
    xEventGroupSetBits(this->temperature_wifi_event_group, WIFI_CONNECTED_BIT);
    break;
  }
  default:
    break;
  }
}

bool Temperature_wifi::get_is_connected()
{
  return this->is_connected;
}

void Temperature_wifi::consume_wifi_event(int32_t id, void *event_data)
{
  switch (id)
  {
    case WIFI_EVENT_STA_START:
    {
      ESP_LOGI(TAG, "Event station start");
      this->connect();
      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGI(TAG, "Event station disconnected");
      if (s_retry_count < WIFI_MAXIUM_RETRY)
      {
        this->connect();
        s_retry_count++;
        ESP_LOGI(TAG, "Retry to connect to the AP");
      }
      else
      {
        xEventGroupSetBits(this->temperature_wifi_event_group, WIFI_FAIL_BIT);
      }
      ESP_LOGI(TAG, "connect to the AP fail");
      break;
    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "Event Station connected");
      break;
    default:
      ESP_LOGI(TAG, "Other wifi event, id: %li", id);
      break;
    }
}

void Temperature_wifi::connect()
{
  esp_err_t return_code = esp_wifi_connect();
  //only if something went wrong
  if (return_code != ESP_OK)
  {
    this->log_err_code(return_code, "Wifi connect fail");
  }
}

void Temperature_wifi::event_group_wait()
{
  EventBits_t bits = xEventGroupWaitBits(this->temperature_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT)
  {
    ESP_LOGI(TAG, "Connected to ap");
    this->mqtt_client->connect_mqtt();
  }
  else if (bits & WIFI_FAIL_BIT)
  {
    ESP_LOGI(TAG, "Failed to connect to ap");
  }
  else
  {
    ESP_LOGI(TAG, "Unexpected event");
  }
}

void Temperature_wifi::log_err_code(esp_err_t error_code, const char *message)
{
  const char *code_name = esp_err_to_name(error_code);
  ESP_LOGW(TAG, "Return Code: %s, from: %s", code_name, message);
}
