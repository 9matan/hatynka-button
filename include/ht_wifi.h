#pragma once

#include "esp_err.h"

#ifdef __cplusplus
    extern "C" {
#endif

esp_err_t ht_wifi_sta_init();
void ht_wifi_sta_connect(const char* ssid, const char* password);

#ifdef __cplusplus
    }
#endif