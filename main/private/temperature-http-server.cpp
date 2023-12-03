#include "temperature-http-server.h"

static const char *TAG = "temperature_http_server";

static Temperature_http_server *server_instance;

static esp_err_t check_server_instance()
{
  if (server_instance == NULL)
  {
    ESP_LOGI(TAG, "Server instance is null or invalid");
    return ESP_ERR_INVALID_STATE;
  }
  return ESP_OK;
}

static esp_err_t get_index_handler_wrapper(httpd_req_t *req)
{
  ESP_ERROR_CHECK(check_server_instance());
  return server_instance->get_index_handler(req);
}

static esp_err_t get_status_handler_wrapper(httpd_req_t *req)
{
  ESP_ERROR_CHECK(check_server_instance());
  return server_instance->get_status_handler(req);
}

static esp_err_t post_settings_handler_wrapper(httpd_req_t *req)
{
  ESP_ERROR_CHECK(check_server_instance());
  return server_instance->post_settings_handler(req);
}

static esp_err_t get_settings_handler_wrapper(httpd_req_t *req)
{
  ESP_ERROR_CHECK(check_server_instance());
  return server_instance->get_settings_handler(req);
}

Temperature_http_server::Temperature_http_server(void (*status)(cJSON *json), Temperature_preferences *preferences)
{
  this->config = HTTPD_DEFAULT_CONFIG();
  this->config.uri_match_fn = httpd_uri_match_wildcard;
  this->device_status = status;
  this->preferences = preferences;
  server_instance = this;
}

Temperature_http_server::~Temperature_http_server()
{
  httpd_stop(this->server);
}

esp_err_t Temperature_http_server::start_server()
{
  ESP_LOGI(TAG, "Start http server");
  return httpd_start(&this->server, &config);
}

esp_err_t Temperature_http_server::register_endpoints()
{
  ESP_LOGI(TAG, "Register http handlers");

  httpd_uri_t index_get_uri = {
      .uri = "/index",
      .method = HTTP_GET,
      .handler = get_index_handler_wrapper,
      .user_ctx = NULL};

  httpd_uri_t device_status_get_uri = {
      .uri = "/api/device-status",
      .method = HTTP_GET,
      .handler = get_status_handler_wrapper,
      .user_ctx = NULL};

  httpd_uri_t settings_post_uri = {
      .uri = "/api/settings",
      .method = HTTP_POST,
      .handler = post_settings_handler_wrapper,
      .user_ctx = NULL};

  httpd_uri_t settings_get_uri = {
      .uri = "/api/settings",
      .method = HTTP_GET,
      .handler = get_settings_handler_wrapper,
      .user_ctx = NULL};
  httpd_register_uri_handler(this->server, &index_get_uri);
  httpd_register_uri_handler(this->server, &device_status_get_uri);
  httpd_register_uri_handler(this->server, &settings_post_uri);
  httpd_register_uri_handler(this->server, &settings_get_uri);

  return ESP_OK;
}

esp_err_t Temperature_http_server::get_index_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "Index handler invoked");
  const char *response = "Hello world";

  httpd_resp_send(req, response, strlen(response));
  return ESP_OK;
}

esp_err_t Temperature_http_server::get_status_handler(httpd_req_t *req)
{
  cJSON *json = cJSON_CreateObject();
  this->device_status(json);
  const char *content = cJSON_Print(json);
  httpd_resp_set_type(req, HTTPD_TYPE_JSON);
  httpd_resp_send(req, content, strlen(content));
  return ESP_OK;
}

esp_err_t Temperature_http_server::post_settings_handler(httpd_req_t *req)
{
  size_t size = req->content_len;
  char *buf = (char *)malloc(size);
  httpd_req_recv(req, buf, size);
  return ESP_OK;
}

esp_err_t Temperature_http_server::get_settings_handler(httpd_req_t *req)
{
  models::Temperature_preferences_t config;
  this->preferences->load_preferences(&config);
  nlohmann::json json = config;
  const char *content = json.dump().c_str();
  ESP_LOGI(TAG, "Content: %s", content);
  httpd_resp_set_type(req, HTTPD_TYPE_JSON);
  httpd_resp_send(req, content, strlen(content));
  return ESP_OK;
}
