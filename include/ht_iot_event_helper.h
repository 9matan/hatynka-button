#pragma once

#include "ht_iot_event.h"

#ifdef __cplusplus
    extern "C" {
#endif

struct cJSON* ht_iot_event_to_json(ht_iot_event_t event);
const char* ht_iot_device_type_to_string(ht_iot_device_type_t device_type);
const char* ht_iot_event_type_to_string(ht_iot_device_type_t device_type, uint8_t event_type);

#ifdef __cplusplus
    }
#endif