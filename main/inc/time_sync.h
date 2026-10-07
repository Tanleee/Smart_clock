#pragma once

#include "esp_err.h"

/* Tạo task đồng bộ giờ qua SNTP rồi gửi giờ hiện tại vào queue display */
esp_err_t time_sync_start(void);
