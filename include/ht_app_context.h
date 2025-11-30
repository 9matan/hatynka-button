#pragma once

#include <stddef.h>

#include "esp_err.h"

#define HT_HTTP_HOST_MAX_SIZE 64
#define HT_GUID_SIZE 37

typedef struct
{
    char guid[HT_GUID_SIZE];
} ht_guid;

typedef void* ht_app_cntx_handle_t;

#ifdef __cplusplus
    extern "C" {
#endif

ht_app_cntx_handle_t ht_app_cntx_create(ht_guid const* device_guid, uint16_t port);
void ht_app_cntx_delete(ht_app_cntx_handle_t app_cntx_h);

ht_guid const* ht_app_cntx_get_device_guid(ht_app_cntx_handle_t app_cntx_h);

esp_err_t ht_app_cntx_set_iot_hub_host(ht_app_cntx_handle_t app_cntx_h, char const* host, size_t host_size);
esp_err_t ht_app_cntx_get_iot_hub_host(ht_app_cntx_handle_t app_cntx_h, char* host, size_t host_size);

uint16_t ht_app_cntx_get_iot_hub_http_port(ht_app_cntx_handle_t app_cntx_h);

#ifdef __cplusplus
    }
#endif