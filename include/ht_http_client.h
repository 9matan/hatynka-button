#pragma once

#include "esp_err.h"

#ifdef __cplusplus
    extern "C" {
#endif

esp_err_t ht_http_client_init();
int ht_http_client_post_request(const char* path,
    const char* data,
    const char* http_host,
    int http_port);

#ifdef __cplusplus
    }
#endif