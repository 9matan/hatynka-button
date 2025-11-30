#include "esp_log.h"

#include "cJSON.h"

#include "ht_iot_button_event_types.h"
#include "ht_iot_event.h"
#include "ht_iot_event_helper.h"

static const char* TAG = "ht_iot_event"; 

static const char* iot_button_event_type_to_string(ht_iot_button_event_type_t event_type);

cJSON* ht_iot_event_to_json(ht_iot_event_t iot_event)
{
    cJSON* json_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(json_obj, "device_type", ht_iot_device_type_to_string(iot_event.device_type));
    cJSON_AddStringToObject(json_obj, "event_type", ht_iot_event_type_to_string(iot_event.device_type, iot_event.event_type));
    return json_obj;
}

const char* ht_iot_device_type_to_string(ht_iot_device_type_t device_type)
{
    switch(device_type)
    {
    case HT_IOT_DEVICE_BUTTON:
        return "button";
    default:
        ESP_LOGE(TAG, "Unhandled device type: %i", (int)device_type);
        return "none";
    }
}

const char* ht_iot_event_type_to_string(ht_iot_device_type_t device_type, uint8_t event_type)
{
    switch(device_type)
    {
    case HT_IOT_DEVICE_BUTTON:
        return iot_button_event_type_to_string((ht_iot_button_event_type_t)event_type);
    default:
        ESP_LOGE(TAG, "Unhandled device type: %i", (int)device_type);
        return "none";
    }
}

static const char* iot_button_event_type_to_string(ht_iot_button_event_type_t event_type)
{
    switch(event_type)
    {
    case HT_IOT_BUTTON_EVENT_CLICK:
        return "click";
    default:
        ESP_LOGE(TAG, "Unhandled button iot event type: %i", (int)event_type);
        return "none";
    }
}