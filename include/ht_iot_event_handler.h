#pragma once

#include "freertos/queue.h"

#include "esp_err.h"

#include "ht_app_context.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef void* ht_iot_event_handler_handle_t;

esp_err_t ht_iot_event_handler_init(ht_app_cntx_handle_t app_cntx);
ht_iot_event_handler_handle_t ht_iot_event_handler_create(
    QueueHandle_t iot_event_queue);

#ifdef __cplusplus
    }
#endif