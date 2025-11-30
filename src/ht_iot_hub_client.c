#include <string.h>

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "ht_iot_hub_client.h"
#include "ht_udp_server.h"

typedef enum
{
    HT_IOT_HUB_PROTOCOL_INVALID = 0,
    HT_IOT_HUB_PROTOCOL_BEACON
} ht_iot_hub_protocol_type_t;

typedef struct
{
    ht_ip_addr_t ip_addr;
    char* buffer;
    size_t buffer_len;
    size_t message_len;
    ht_iot_hub_protocol_type_t protocol_type;
} iot_hub_message_t;

typedef struct
{
    ht_iot_hub_client_host_discovery_cb_t host_discovery_cb;
    int beacon_receiver_id;
    ht_ip_addr_t discovered_host;
} ht_iot_hub_client_t;

static const char* TAG = "ht_iot_hub_client";

static int create_beacon_receiver(uint16_t beacon_port);
static bool update_beacon_receiver(ht_iot_hub_client_t* client);
static esp_err_t handle_beacon_message(int server_id, iot_hub_message_t* message);
static esp_err_t iot_hub_client_read_msg(int server_id, iot_hub_message_t* message);
static ht_iot_hub_protocol_type_t iot_hub_client_get_protocol(const char* data);

ht_iot_hub_client_handler_t ht_iot_hub_client_create(uint16_t beacon_port)
{
    int beacon_receiver_id = create_beacon_receiver(beacon_port);
    if (beacon_receiver_id < 0)
    {
        return NULL;
    }

    ht_iot_hub_client_t* hub_client = (ht_iot_hub_client_t*)calloc(1, sizeof(ht_iot_hub_client_t));
    hub_client->beacon_receiver_id = beacon_receiver_id;
    return (ht_iot_hub_client_handler_t)hub_client;
}

void ht_iot_hub_client_set_host_discovery_cb(
    ht_iot_hub_client_handler_t client_h,
    ht_iot_hub_client_host_discovery_cb_t cb)
{
    assert(client_h);

    ht_iot_hub_client_t* client = (ht_iot_hub_client_t*)client_h;
    client->host_discovery_cb = cb;
}

bool ht_iot_hub_client_update(ht_iot_hub_client_handler_t client_h)
{
    assert(client_h);

    ht_iot_hub_client_t* client = (ht_iot_hub_client_t*)client_h;
    return update_beacon_receiver(client);
}

static int create_beacon_receiver(uint16_t beacon_port)
{
    ht_udp_server_config_t server_config;
    memset(&server_config, 0, sizeof(ht_udp_server_config_t));
    strcpy(server_config.ip_addr.addr, "0.0.0.0");
    server_config.port = beacon_port;
    server_config.non_blocking = true;
    server_config.recv_timeout_s = 0;

    int server_id = ht_udp_server_start(&server_config);
    if (server_id < 0)
    {
        ESP_LOGE(TAG, "Could not create beacon udp server");
        return -1;
    }
    return server_id;
}

static bool update_beacon_receiver(ht_iot_hub_client_t* client)
{
    assert(client);

    iot_hub_message_t message;
    memset(&message, 0, sizeof(iot_hub_message_t));
    message.buffer_len = 32;
    message.buffer = (char*) calloc(1, message.buffer_len);

    bool is_updated = false;
    esp_err_t read_msg_err = handle_beacon_message(client->beacon_receiver_id, &message);
    if (read_msg_err == ESP_OK)
    {
        is_updated = true;
        if (strcmp(message.ip_addr.addr, client->discovered_host.addr) != 0)
        {
            ESP_LOGI(TAG, "Discovered a new hub: %s", message.ip_addr.addr);
            if (client->host_discovery_cb)
            {
                strcpy(client->discovered_host.addr, message.ip_addr.addr);
                client->host_discovery_cb(client, &(client->discovered_host));
            }
        }
    }
    free(message.buffer);
    return is_updated;
}

static esp_err_t handle_beacon_message(int server_id, iot_hub_message_t* message)
{
    esp_err_t err = iot_hub_client_read_msg(server_id, message);
    if (err != ESP_OK)
    {
        if (err != ESP_ERR_TIMEOUT)
        {
            ESP_LOGE(TAG, "Could not get a message, err: %i", err);
        }
        return err;
    }

    if (message->protocol_type != HT_IOT_HUB_PROTOCOL_BEACON)
    {
        ESP_LOGE(TAG, "Unexpected protocol: %i. Message: %*.s", message->protocol_type, 16, message->buffer);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t iot_hub_client_read_msg(int server_id, iot_hub_message_t* message)
{
    assert(message);
    assert(server_id >= 0);

    ht_udp_message_t udp_message;
    memset(&udp_message, 0, sizeof(ht_udp_message_t));
    udp_message.buffer = message->buffer;
    udp_message.buffer_len = message->buffer_len;
    esp_err_t err = ht_udp_server_recv(server_id, &udp_message);
    if (err != 0)
    {
        ESP_LOGE(TAG, "ht_udp_server_recv failed, err: %i", err);
        return err;
    }

    ESP_LOGD(TAG, "Read a message: %*.s", 32, udp_message.buffer);
    message->protocol_type = iot_hub_client_get_protocol(message->buffer);
    strcpy(message->ip_addr.addr, udp_message.ip_addr.addr);
    return ESP_OK;
}

static ht_iot_hub_protocol_type_t iot_hub_client_get_protocol(const char* data)
{
    char* separator = strstr(data, " ");
    size_t protocol_size = separator
        ? (size_t)(separator - data)
        : strlen(data);

    if (strncmp(data, "HT_HUB_BEACON", protocol_size) == 0)
    {
        return HT_IOT_HUB_PROTOCOL_BEACON;
    }

    return HT_IOT_HUB_PROTOCOL_INVALID;
}