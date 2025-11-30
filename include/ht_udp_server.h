#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "esp_err.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define HT_IP_ADDR_LENGTH 16 // INET_ADDRSTRLEN

typedef struct ht_ip_addr_t
{
    char addr[HT_IP_ADDR_LENGTH];
} ht_ip_addr_t;

typedef struct
{
    ht_ip_addr_t ip_addr;
    uint16_t port;
    time_t recv_timeout_s;
    bool non_blocking;
} ht_udp_server_config_t;

typedef struct
{
    ht_ip_addr_t ip_addr;
    char* buffer;
    size_t buffer_len;
    size_t read_len;
} ht_udp_message_t;

esp_err_t ht_udp_server_start(ht_udp_server_config_t* config);
esp_err_t ht_udp_server_recv(int server_id, ht_udp_message_t* message);
esp_err_t ht_udp_server_stop(int server_id);

#ifdef __cplusplus
    }
#endif