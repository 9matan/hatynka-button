#include <string.h>

#include "esp_http_client.h"
#include "esp_log.h"

#define CLEANUP_HTTP_CLIENT(http_client) ESP_ERROR_CHECK(esp_http_client_cleanup(http_client))

static const char* TAG = "ht_http_client";

static const int HTTP_REQUEST_DEFAULT_TIMEOUT_MS = 10000;

static char s_http_user_agent[128];

static esp_err_t http_client_event_handler(esp_http_client_event_t *evt);

esp_err_t ht_http_client_init()
{
    strcpy(s_http_user_agent, "hatynka/0.0.1 esp-idf/");
    strcat(s_http_user_agent, esp_get_idf_version());
    return ESP_OK;
}

int ht_http_client_post_request(const char* path,
    const char* data,
    const char* http_host,
    int http_port)
{
    ESP_LOGI(TAG, "Sending the post request: %s:%u/%s", http_host, http_port, path);

    esp_http_client_config_t http_client_config = {
        .host = http_host,
        .port = http_port,
        .user_agent = s_http_user_agent,
        .method = HTTP_METHOD_POST,
        .path = path,
        .event_handler = http_client_event_handler,
        .timeout_ms = HTTP_REQUEST_DEFAULT_TIMEOUT_MS
    };

    esp_http_client_handle_t http_client = esp_http_client_init(&http_client_config);
    if (!http_client)
    {
        ESP_LOGE(TAG, "Could not create a HTTP client");
        CLEANUP_HTTP_CLIENT(http_client);
        return -1;
    }

    if (esp_http_client_set_header(http_client,
                                   "Content-Type",
                                   "application/json") != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set Content-Type of a request");
        CLEANUP_HTTP_CLIENT(http_client);
        return -1;
    }

    if (esp_http_client_set_post_field(http_client,
                                       data,
                                       strlen(data)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set post data of a request");
        CLEANUP_HTTP_CLIENT(http_client);
        return -1;
    }

    if (esp_http_client_perform(http_client) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to perfom the request");
        CLEANUP_HTTP_CLIENT(http_client);
        return -1;
    }

    const int status_code = esp_http_client_get_status_code(http_client);
    ESP_LOGI(TAG, "Response status code: %i", status_code);
    CLEANUP_HTTP_CLIENT(http_client);
    return status_code;
}

static esp_err_t http_client_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}