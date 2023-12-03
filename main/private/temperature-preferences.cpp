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

    handle->get_item(temperature_preferences_keys.interval, preferences->measure_intervall);
    handle->get_item(temperature_preferences_keys.mqtt_port, preferences->mqtt_port);
    preferences->mqtt_host = this->read_string(handle.get(), temperature_preferences_keys.mqtt_host);
    preferences->name = this->read_string(handle.get(), temperature_preferences_keys.mqtt_name);
    preferences->room =this->read_string(handle.get(), temperature_preferences_keys.mqtt_room);
    preferences->ssid = this->read_string(handle.get(), temperature_preferences_keys.wifi_ssid);
    preferences->password = this->read_string(handle.get(), temperature_preferences_keys.wifi_password);
    ESP_LOGI(TAG, "Preferences loaded succesfully");
    return ESP_OK;
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

esp_err_t Temperature_preferences::save_prefrenecs(models::Temperature_preferences_t *preferences)
{
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_NAMESPACE, NVS_READWRITE, &err);
    ESP_ERROR_CHECK(err);

    handle->set_item(temperature_preferences_keys.interval, preferences->measure_intervall);
    handle->set_item(temperature_preferences_keys.mqtt_port, preferences->mqtt_port);
    handle->set_string(temperature_preferences_keys.mqtt_host, preferences->mqtt_host);
    handle->set_string(temperature_preferences_keys.mqtt_name, preferences->name);
    handle->set_string(temperature_preferences_keys.mqtt_room, preferences->room);
    handle->set_string(temperature_preferences_keys.wifi_ssid, preferences->ssid);
    handle->set_string(temperature_preferences_keys.wifi_password, preferences->password);
    handle->commit();
    return ESP_OK;
}

esp_err_t Temperature_preferences::set_factory_default(int force_factory)
{
    models::Temperature_preferences_t preferences = { };
    preferences.ssid = WIFI_SSID;
    preferences.password = WIFI_PASSWORD;
    preferences.mqtt_host = CONFIG_BROKER_HOST;
    preferences.room = CONFIG_ROOM;
    preferences.name = CONFIG_NAME;
    preferences.mqtt_port = CONFIG_BROKER_PORT;
    preferences.measure_intervall = INTERVAL;

    //this->save_prefrenecs(&preferences);
    return ESP_OK;
}
