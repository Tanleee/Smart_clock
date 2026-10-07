#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "esp_netif.h"

/* Khởi tạo và bật WiFi STA (kết nối bằng WIFI_SSID / WIFI_PASS trong app_config.h) */
void wifi_sta_init(void);

/* Chờ kết quả kết nối. Trả về true nếu đã có IP, false nếu thất bại hoặc hết thời gian chờ */
bool wifi_sta_wait_connected(TickType_t timeout);

/* Lấy netif của STA (dùng để cấu hình DNS...) */
esp_netif_t *wifi_sta_get_netif(void);
