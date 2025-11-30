#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"

static const int WIFI_MAXIMUM_CONN_RETRY = 10;
static const EventBits_t WIFI_CONNECTED_BIT = BIT0;
static const EventBits_t WIFI_FAIL_BIT = BIT1;

static const char* TAG = "ht_wifi";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static void wifi_sta_wait_for_connection();
static void got_ip_event_handler(void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data);
static void wifi_event_handler(void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data);

esp_err_t ht_wifi_sta_init()
{
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    return ESP_OK;
}

void ht_wifi_sta_connect(const char* ssid, const char* password)
{
    ESP_LOGI(TAG, "Connecting to the AP");

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK
        }
    };
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, password);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_wifi_event_group = xEventGroupCreate();
    assert(s_wifi_event_group);
    esp_event_handler_instance_t wifi_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_STA_DISCONNECTED,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &wifi_event_instance));
    esp_event_handler_instance_t got_ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &got_ip_event_handler,
                                                        NULL,
                                                        &got_ip_event_instance));

    ESP_ERROR_CHECK(esp_wifi_connect());
    wifi_sta_wait_for_connection();

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_instance));
    vEventGroupDelete(s_wifi_event_group);
}

static void wifi_event_handler(void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data)
{
    assert(event_base == WIFI_EVENT);
    assert(event_id == WIFI_EVENT_STA_DISCONNECTED);

    if (s_retry_num < WIFI_MAXIMUM_CONN_RETRY)
    {
        ESP_LOGD(TAG, "retrying to connect to the AP");
        esp_wifi_connect();
        s_retry_num++;
    }
    else
    {
        ESP_LOGE(TAG, "connection to the AP has been failed");
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
}

static void got_ip_event_handler(void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data)
{
    assert(event_base == IP_EVENT);
    assert(event_id == IP_EVENT_STA_GOT_IP);
    
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
}

static void wifi_sta_wait_for_connection()
{
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to the AP");
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect the AP");
    }
    else
    {
        ESP_LOGE(TAG, "Unexpected event");
    }
}