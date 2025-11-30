#pragma once

#include <stdint.h>

#include <ht_iot_device_types.h>

typedef struct
{
    ht_iot_device_type_t device_type;
    uint8_t event_type;
} ht_iot_event_t;