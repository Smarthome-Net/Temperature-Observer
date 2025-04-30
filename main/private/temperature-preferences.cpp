#include "temperature-preferences.h"

static const char *NVS_NAMESPACE = "temp_observer";
static const char *TAG = "temperature_preferences";

//reuseable config keys
const struct temperature_preferences_keys_t {
    const char *interval = "interval";
    const char *mqtt_port = "mqtt_port";
    const char *mqtt_host = "mqtt_host";
    const char *mqtt_name = "mqtt_name";
    const char *mqtt_room = "mqtt_room";
    const char *wifi_ssid = "wifi_ssid";
    const char *wifi_password = "wifi_password";
} temperature_preferences_keys;

static const int INTERVAL = 10;

#define WIFI_SSID CONFIG_WIFI_SSID

#define WIFI_PASSWORD CONFIG_WIFI_PASSWORD

#define BROKER_HOST CONFIG_BROKER_HOST

#define BROKER_PORT CONFIG_BROKER_PORT

#define ROOM CONFIG_ROOM

#define NAME CONFIG_NAME

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



Temperature_preferences::Temperature_preferences()
{
}

Temperature_preferences::~Temperature_preferences()
{
}

esp_err_t Temperature_preferences::load_preferences(models::Temperature_preferences_t *preferences)
{
    ESP_LOGI(TAG, "Load preferences");
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_NAMESPACE, NVS_READONLY, &err);
    ESP_ERROR_CHECK(err);

    handle->get_item(temperature_preferences_keys.interval, preferences->measure_interval);
    handle->get_item(temperature_preferences_keys.mqtt_port, preferences->mqtt_port);
    preferences->mqtt_host = this->read_string(handle.get(), temperature_preferences_keys.mqtt_host);
    preferences->device_name = this->read_string(handle.get(), temperature_preferences_keys.mqtt_name);
    preferences->room =this->read_string(handle.get(), temperature_preferences_keys.mqtt_room);
    preferences->ssid = this->read_string(handle.get(), temperature_preferences_keys.wifi_ssid);
    preferences->password = this->read_string(handle.get(), temperature_preferences_keys.wifi_password);
    ESP_LOGI(TAG, "Preferences loaded succesfully");
    return err;
}

esp_err_t Temperature_preferences::load_wifi_config(wifi_config_t *wifi_config) 
{
    ESP_LOGI(TAG, "Load wifi config");
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_NAMESPACE, NVS_READONLY, &err);
    ESP_ERROR_CHECK(err);

    strcpy((char*)wifi_config->sta.ssid, this->read_string(handle.get(), temperature_preferences_keys.wifi_ssid));
    strcpy((char*)wifi_config->sta.password, this->read_string(handle.get(), temperature_preferences_keys.wifi_password));
    wifi_config->sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config->sta.pmf_cfg.capable = true;
    wifi_config->sta.pmf_cfg.required = false;
    ESP_LOGI(TAG, "Wifi config loaded succesfully");
    return err;
}

esp_err_t Temperature_preferences::load_mqtt_config(models::Temperature_mqtt_config_t *mqtt_config)
{
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_NAMESPACE, NVS_READONLY, &err);
    ESP_ERROR_CHECK(err);

    mqtt_config->room = this->read_string(handle.get(), temperature_preferences_keys.mqtt_room);
    mqtt_config->device_name = this->read_string(handle.get(), temperature_preferences_keys.mqtt_name);
    esp_mqtt_client_config_t config = {};
    config.broker.address.hostname = this->read_string(handle.get(), temperature_preferences_keys.mqtt_host);
    config.broker.address.port = this->read_uint32_t(handle.get(), temperature_preferences_keys.mqtt_port);
    config.broker.address.transport = TRANSPORT;
    mqtt_config->mqtt_config = config;
    return err;
}

esp_err_t Temperature_preferences::load_intervall(uint32_t *intervall)
{
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_NAMESPACE, NVS_READONLY, &err);
    ESP_ERROR_CHECK(err);

    handle->get_item(temperature_preferences_keys.interval, *intervall);
    return err;
}

char *Temperature_preferences::read_string(nvs::NVSHandle *handle, const char *key)
{
    size_t size;
    char *value;
    ESP_ERROR_CHECK(handle->get_item_size(nvs::ItemType::SZ, key, size));
    value = (char *)malloc(size); //assign size to value, otherwise we get panic
    ESP_ERROR_CHECK(handle->get_string(key, value, size));
    return value;
}

uint32_t Temperature_preferences::read_uint32_t(nvs::NVSHandle *handle, const char *key)
{
    uint32_t value;
    ESP_ERROR_CHECK(handle->get_item(key, value));
    return value;
}

esp_err_t Temperature_preferences::save_prefrenecs(models::Temperature_preferences_t *preferences)
{
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_NAMESPACE, NVS_READWRITE, &err);
    ESP_ERROR_CHECK(err);

    if(preferences->measure_interval > 0) {
        handle->set_item(temperature_preferences_keys.interval, preferences->measure_interval);
    }

    if(preferences->mqtt_port > 0) {
        handle->set_item(temperature_preferences_keys.mqtt_port, preferences->mqtt_port);
    }

    if(preferences->mqtt_host.length() > 0) {
        handle->set_string(temperature_preferences_keys.mqtt_host, preferences->mqtt_host.c_str());
    }

    if(preferences->device_name.length() > 0) {
        handle->set_string(temperature_preferences_keys.mqtt_name, preferences->device_name.c_str());
    }
    if(preferences->room.length() > 0) {
        handle->set_string(temperature_preferences_keys.mqtt_room, preferences->room.c_str());
    }
    if(preferences->ssid.length() > 0) {
        handle->set_string(temperature_preferences_keys.wifi_ssid, preferences->ssid.c_str());
    }

    if(preferences->password.length() > 0) {
        handle->set_string(temperature_preferences_keys.wifi_password, preferences->password.c_str());
    }
    
    err = handle->commit();
    ESP_ERROR_CHECK(err);
    return err;
}

esp_err_t Temperature_preferences::set_factory_default(int force_factory)
{
    models::Temperature_preferences_t preferences = { };
    preferences.ssid = WIFI_SSID;
    preferences.password = WIFI_PASSWORD;
    preferences.mqtt_host = CONFIG_BROKER_HOST;
    preferences.room = CONFIG_ROOM;
    preferences.device_name = CONFIG_NAME;
    preferences.mqtt_port = CONFIG_BROKER_PORT;
    preferences.measure_interval = INTERVAL;

    this->save_prefrenecs(&preferences);
    return ESP_OK;
}
