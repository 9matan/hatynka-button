#include "ht_udp_server.h"

#include "esp_log.h"
#include "esp_assert.h"

#include "lwip/sockets.h"

static const char* TAG = "ht_udp_server";

esp_err_t ht_udp_server_start(ht_udp_server_config_t* config)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }
    ESP_LOGI(TAG, "Socket created");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->port);
    inet_pton(AF_INET, config->ip_addr.addr, &(server_addr.sin_addr));

    if (!config->non_blocking)
    {
        struct timeval timeout;
        timeout.tv_sec = config->recv_timeout_s > 0 ? config->recv_timeout_s : 0;
        timeout.tv_usec = config->recv_timeout_s > 0 ? 0 : 10;
        int err = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
        if (err != 0)
        {
            ESP_LOGE(TAG, "Could not set socket timeout: errno %d", err);
            goto error;
        }
    }

    int err = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err < 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto error;
    }

    ESP_LOGI(TAG, "Socket bound, id: %i, port: %u", sock, config->port);
    return sock;

error:
    close(sock);
    return -1;
}

esp_err_t ht_udp_server_recv(int server_id, ht_udp_message_t* message)
{
    assert(message);
    assert(message->buffer);
    assert(message->buffer_len > 0);

    struct sockaddr src_addr;
    memset(&src_addr, 0, sizeof(src_addr));
    socklen_t socklen = sizeof(src_addr);

    ssize_t len = recvfrom(server_id, message->buffer, message->buffer_len - 1, 0, &src_addr, &socklen);
    if (len < 0)
    {
        ESP_LOGE(TAG, "recvfrom failed, id: %i, err: %i", server_id, errno);
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return ESP_ERR_TIMEOUT;
        }
        return ESP_FAIL;
    }

    struct sockaddr_in* src_addr_in = (struct sockaddr_in *)(&src_addr);
    inet_ntop(AF_INET, &(src_addr_in->sin_addr), message->ip_addr.addr, HT_IP_ADDR_LENGTH);

    message->read_len = (size_t)len;
    message->buffer[len] = '\0';
    return ESP_OK;
}

esp_err_t ht_udp_server_stop(int server_id)
{
    if (server_id >= 0)
    {
        shutdown(server_id, 0);
        if (close(server_id) != 0)
        {
            ESP_LOGE(TAG, "Unable to close socket: errno %d", errno);
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Socket closed");
    }
    return ESP_OK;
}