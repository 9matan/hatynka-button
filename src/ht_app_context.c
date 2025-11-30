#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_check.h"

#include "ht_app_context.h"

static const char* TAG = "ht_app_context";

typedef struct
{
    char iot_hub_host[HT_HTTP_HOST_MAX_SIZE];
    ht_guid device_guid;
    SemaphoreHandle_t app_cntx_mutex;
    uint16_t iot_hub_http_port;
} ht_app_cntx_t;

static void app_cntx_lock(ht_app_cntx_t* app);
static void app_cntx_unlock(ht_app_cntx_t* app);

ht_app_cntx_handle_t ht_app_cntx_create(ht_guid const* device_guid, uint16_t iot_hub_http_port)
{
    assert(device_guid);

    ht_app_cntx_t* app_cntx = (ht_app_cntx_t*)calloc(1, sizeof(ht_app_cntx_t));
    strcpy(app_cntx->device_guid.guid, device_guid->guid);
    app_cntx->app_cntx_mutex = xSemaphoreCreateMutex();
    app_cntx->iot_hub_http_port = iot_hub_http_port;

    return (ht_app_cntx_handle_t)app_cntx;
}

void ht_app_cntx_delete(ht_app_cntx_handle_t app_cntx_h)
{
    assert(app_cntx_h);

    ht_app_cntx_t* app_cntx = (ht_app_cntx_t*)app_cntx_h;
    vSemaphoreDelete(app_cntx->app_cntx_mutex);
    free(app_cntx);
}

ht_guid const* ht_app_cntx_get_device_guid(ht_app_cntx_handle_t app_cntx_h)
{
    assert(app_cntx_h);

    ht_app_cntx_t* app_cntx = (ht_app_cntx_t*)app_cntx_h;
    return &(app_cntx->device_guid);
}

esp_err_t ht_app_cntx_set_iot_hub_host(ht_app_cntx_handle_t app_cntx_h, char const* host, size_t host_size)
{
    assert(app_cntx_h);

    ESP_RETURN_ON_FALSE(host_size <= HT_HTTP_HOST_MAX_SIZE,
                        ESP_ERR_INVALID_SIZE,
                        TAG, "The host size is too large");
    ht_app_cntx_t* app_cntx = (ht_app_cntx_t*)app_cntx_h;

    app_cntx_lock(app_cntx);
    strcpy(app_cntx->iot_hub_host, host);
    app_cntx_unlock(app_cntx);

    return ESP_OK;
}

esp_err_t ht_app_cntx_get_iot_hub_host(ht_app_cntx_handle_t app_cntx_h, char* host, size_t host_size)
{
    assert(app_cntx_h);

    ht_app_cntx_t* app_cntx = (ht_app_cntx_t*)app_cntx_h;
    size_t app_cntx_host_size = strlen(app_cntx->iot_hub_host);
    ESP_RETURN_ON_FALSE(app_cntx_host_size <= host_size,
                        ESP_ERR_INVALID_SIZE,
                        TAG, "The host size is too small");

    app_cntx_lock(app_cntx);
    strcpy(host, app_cntx->iot_hub_host);
    app_cntx_unlock(app_cntx);

    return ESP_OK;
}

uint16_t ht_app_cntx_get_iot_hub_http_port(ht_app_cntx_handle_t app_cntx_h)
{
    assert(app_cntx_h);

    ht_app_cntx_t* app_cntx = (ht_app_cntx_t*)app_cntx_h;

    return app_cntx->iot_hub_http_port;
}

static void app_cntx_lock(ht_app_cntx_t* app_cntx)
{
    BaseType_t is_semphr_taken = xSemaphoreTake(app_cntx->app_cntx_mutex, portMAX_DELAY);
    assert(is_semphr_taken == pdPASS);
}

static void app_cntx_unlock(ht_app_cntx_t* app_cntx)
{
    BaseType_t is_semphr_given = xSemaphoreGive(app_cntx->app_cntx_mutex);
    assert(is_semphr_given == pdPASS);
}