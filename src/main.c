#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "ht_app_context.h"
#include "ht_http_client.h"
#include "ht_iot_button.h"
#include "ht_iot_event.h"
#include "ht_iot_event_handler.h"
#include "ht_iot_hub_client.h"
#include "ht_udp_server.h"
#include "ht_wifi.h"

static const char* WIFI_SSID = "SSID";
static const char* WIFI_PASS = "PASSWORD";
static const int HUB_BEACON_PORT = 29921;
static const int HUB_HTTP_PORT = 5000;
static const int IOT_BUTTON_GPIO_NUM = 14;
static const UBaseType_t IOT_EVENT_QUEUE_SIZE = 16;

static QueueHandle_t s_iot_event_queue = NULL;
static ht_app_cntx_handle_t s_app_cntx = NULL;
static ht_iot_button_handle_t s_iot_button = NULL;
static ht_iot_event_handler_handle_t s_iot_event_handler = NULL;
static ht_iot_hub_client_handler_t s_iot_hub_client = NULL;

static void system_init();
static void nvs_init();

static void ht_app_init();
static void ht_app_run();

static void init_app_context();
static void on_new_hub_discovered(ht_iot_hub_client_handler_t client, ht_ip_addr_t const* hub_ip_addr);

void app_main()
{
    system_init();
    ht_app_init();
    ht_app_run();
}

static void system_init()
{
    nvs_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
}

static void ht_app_init()
{
    init_app_context();

    ESP_ERROR_CHECK(ht_wifi_sta_init());
    ESP_ERROR_CHECK(ht_http_client_init());
    ESP_ERROR_CHECK(ht_iot_event_handler_init(s_app_cntx));
}

static void ht_app_run()
{
    ht_wifi_sta_connect(WIFI_SSID, WIFI_PASS);
    
    s_iot_hub_client = ht_iot_hub_client_create(HUB_BEACON_PORT);
    assert(s_iot_hub_client);
    ht_iot_hub_client_set_host_discovery_cb(s_iot_hub_client, on_new_hub_discovered);

    s_iot_event_queue = xQueueCreate(IOT_EVENT_QUEUE_SIZE, sizeof(ht_iot_event_t));
    assert(s_iot_event_queue);
    s_iot_button = ht_iot_button_create(IOT_BUTTON_GPIO_NUM, s_iot_event_queue);
    assert(s_iot_button);
    s_iot_event_handler = ht_iot_event_handler_create(s_iot_event_queue);
    assert(s_iot_event_handler);

    while (true)
    {
        ht_iot_hub_client_update(s_iot_hub_client);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

static void nvs_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void init_app_context()
{
    ht_guid guid;
    strcpy(guid.guid, "00000000-0000-0000-0000-000000000000");
    s_app_cntx = ht_app_cntx_create(&guid, HUB_HTTP_PORT);
    assert(s_app_cntx);
}

static void on_new_hub_discovered(ht_iot_hub_client_handler_t client, ht_ip_addr_t const* hub_ip_addr)
{
    ht_app_cntx_set_iot_hub_host(s_app_cntx, hub_ip_addr->addr, sizeof(hub_ip_addr->addr));
}
