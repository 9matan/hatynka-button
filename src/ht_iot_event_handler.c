#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_task.h"

#include "cJSON.h"

#include "ht_http_client.h"
#include "ht_iot_event.h"
#include "ht_iot_event_handler.h"
#include "ht_iot_event_helper.h"

typedef struct EventHandler
{
    QueueHandle_t iot_event_queue;
    struct EventHandler* next;
} ht_iot_event_handler_t;

static const char* TAG = "ht_iot_event_handler";
static const UBaseType_t TASK_PRIO = ESP_TASK_TIMER_PRIO - 1;
static const uint32_t TASK_STACK_SIZE = 16384 + TASK_EXTRA_STACK_SIZE;
static const TickType_t TASK_DELAY = 100 / portTICK_PERIOD_MS;

static ht_app_cntx_handle_t s_app_cntx;
static TaskHandle_t s_handler_task = NULL;
static ht_iot_event_handler_t* s_handler_head = NULL;
static SemaphoreHandle_t s_list_mutex;

static void iot_event_handler_task(void* params);
static void send_iot_event_by_http(ht_iot_event_t iot_event, char* http_host, uint16_t http_port);
static cJSON* generate_device_data_json();

static void insert_to_list(ht_iot_event_handler_t* handler);
static void lock_list();
static void unlock_list();

esp_err_t ht_iot_event_handler_init(ht_app_cntx_handle_t app_cntx)
{
    assert(!s_handler_task);
    assert(app_cntx);
    s_app_cntx = app_cntx;

    s_list_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_list_mutex, ESP_FAIL, TAG, "Could not create a mutex");

    BaseType_t is_task_created = xTaskCreate(iot_event_handler_task,
                                    "IoT Event handler",
                                    TASK_STACK_SIZE,
                                    NULL,
                                    TASK_PRIO,
                                    &s_handler_task);
    ESP_RETURN_ON_FALSE(is_task_created == pdPASS, ESP_FAIL,
                        TAG, "Could not create an IoT event handler task");

    return ESP_OK;
}

ht_iot_event_handler_handle_t ht_iot_event_handler_create(QueueHandle_t iot_event_queue)
{
    assert(iot_event_queue);

    ht_iot_event_handler_t* handler = (ht_iot_event_handler_t*)calloc(1, sizeof(ht_iot_event_handler_t));
    handler->iot_event_queue = iot_event_queue;
    insert_to_list(handler);

    return (ht_iot_event_handler_handle_t)handler;
}

static void iot_event_handler_task(void* params)
{
    ht_iot_event_t iot_event;
    ht_iot_event_handler_t* cur_handler = NULL;
    char http_host[HT_HTTP_HOST_MAX_SIZE];
    uint16_t http_port = 0;

    while(true)
    {
        vTaskDelay(TASK_DELAY);

        ESP_ERROR_CHECK(ht_app_cntx_get_iot_hub_host(s_app_cntx,
                                                http_host,
                                                HT_HTTP_HOST_MAX_SIZE));
        http_port = ht_app_cntx_get_iot_hub_http_port(s_app_cntx);

        lock_list();
        cur_handler = s_handler_head;
        while(cur_handler)
        {
            if (xQueueReceive(cur_handler->iot_event_queue,
                              (void*)&iot_event,
                              0) == pdPASS)
            {
                send_iot_event_by_http(iot_event, http_host, http_port);
            }
            cur_handler = cur_handler->next;
        }
        unlock_list();
    }
}

static void send_iot_event_by_http(ht_iot_event_t iot_event, char* http_host, uint16_t http_port)
{        
    if (http_host[0] == '\0')
    {
        ESP_LOGE(TAG, "Can't send an iot event: http host is empty");
        return;
    }

    cJSON* json_request = cJSON_CreateObject();
    cJSON_AddItemToObject(json_request,
                            "device",
                            generate_device_data_json());
    cJSON_AddItemToObject(json_request,
                          "iot_event",
                          ht_iot_event_to_json(iot_event));

    char* request = cJSON_PrintUnformatted(json_request);
    cJSON_Delete(json_request);

    ht_http_client_post_request("iot_event/add",
                                request,
                                http_host,
                                http_port);

    free(request);
}

static cJSON* generate_device_data_json()
{
    cJSON* device_json = cJSON_CreateObject();
    cJSON_AddStringToObject(device_json,
                            "guid",
                            ht_app_cntx_get_device_guid(s_app_cntx)->guid);
    return device_json;
}

static void insert_to_list(ht_iot_event_handler_t* handler)
{
    assert(handler);

    lock_list();
    handler->next = s_handler_head;
    s_handler_head = handler;
    unlock_list();
}

static void lock_list()
{
    BaseType_t is_semphr_taken = xSemaphoreTake(s_list_mutex, portMAX_DELAY);
    assert(is_semphr_taken == pdPASS);
}

static void unlock_list()
{
    BaseType_t is_semphr_given = xSemaphoreGive(s_list_mutex);
    assert(is_semphr_given == pdPASS);
}