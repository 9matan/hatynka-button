#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "esp_err.h"

#pragma once

#ifdef __cplusplus
    extern "C" {
#endif

struct ht_ip_addr_t;

typedef void* ht_iot_hub_client_handler_t;
typedef void (*ht_iot_hub_client_host_discovery_cb_t)(ht_iot_hub_client_handler_t client_h, struct ht_ip_addr_t const* hub_ip_addr);

ht_iot_hub_client_handler_t ht_iot_hub_client_create(uint16_t beacon_port);
void ht_iot_hub_client_set_host_discovery_cb(
    ht_iot_hub_client_handler_t client_h,
    ht_iot_hub_client_host_discovery_cb_t cb);
bool ht_iot_hub_client_update(ht_iot_hub_client_handler_t client_h);

#ifdef __cplusplus
    }
#endif