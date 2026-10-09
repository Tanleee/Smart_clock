#pragma once
#include "esp_err.h"

/* Tạo task kiểm tra báo thức định kỳ, so khớp giờ hệ thống với alarm_data.
 * Gọi SAU alarm_data_init() + display_init() (và nên sau time_sync_start()). */
esp_err_t alarm_task_start(void);