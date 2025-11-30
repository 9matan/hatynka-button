#pragma once

#include "freertos/queue.h"

#include "esp_err.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef void* ht_iot_button_handle_t;

ht_iot_button_handle_t ht_iot_button_create(
    int gpio_num,
    QueueHandle_t iot_event_queue);

#ifdef __cplusplus
    }
#endif