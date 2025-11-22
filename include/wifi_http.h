#pragma once

#include "esp_err.h"

esp_err_t wifi_init_apsta(const char *ap_ssid, const char *ap_pass);
esp_err_t http_server_start(void);
