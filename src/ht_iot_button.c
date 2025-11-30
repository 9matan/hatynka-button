#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "iot_button.h"

#include "ht_iot_button.h"
#include "ht_iot_button_event_types.h"
#include "ht_iot_event.h"

typedef struct
{
    button_handle_t iot_button;
    QueueHandle_t iot_event_queue;
} ht_iot_button_t;

static const char* TAG = "ht_iot_button";
static const TickType_t QUEUE_SEND_MAX_DELAY = 10 / portTICK_PERIOD_MS;

static void on_iot_button_single_click(void* btn, void* data);

ht_iot_button_handle_t ht_iot_button_create(
    int gpio_num,
    QueueHandle_t iot_event_queue)
{
    assert(iot_event_queue);

    ht_iot_button_t* button = (ht_iot_button_t*)calloc(1, sizeof(ht_iot_button_t));
    button->iot_event_queue = iot_event_queue;

    button_config_t button_config = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .active_level = 0,
            .gpio_num = gpio_num
        }
    };
    button->iot_button = iot_button_create(&button_config);

    ESP_ERROR_CHECK(iot_button_register_cb(button->iot_button,
                                           BUTTON_SINGLE_CLICK,
                                           on_iot_button_single_click,
                                           (void*)button));

    return (ht_iot_button_handle_t)button;
}

static void on_iot_button_single_click(void* btn, void* data)
{
    assert(data);
    ht_iot_button_t* button = (ht_iot_button_t*)data;

    ht_iot_event_t event = {
        .device_type = HT_IOT_DEVICE_BUTTON,
        .event_type = HT_IOT_BUTTON_EVENT_CLICK
    };
    if (xQueueSend(button->iot_event_queue,
                   (void*)&event,
                   QUEUE_SEND_MAX_DELAY) != pdPASS)
    {
        ESP_LOGE(TAG, "Could not send the iot event to the queue");
    }
}