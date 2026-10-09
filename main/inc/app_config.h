#pragma once

/* ====== WiFi ====== */
#define WIFI_SSID          "C427"
#define WIFI_PASS          "64546743"
#define WIFI_MAX_RETRY     10

/* ====== Time ====== */
#define TIME_ZONE          "ICT-7"        /* Việt Nam: UTC+7 */
#define PRINT_PERIOD_MS    1000

/* ====== Task / Queue ====== */
#define TIME_TASK_STACK     4096
#define TIME_TASK_PRIO      5
#define DISPLAY_TASK_STACK  3072
#define DISPLAY_TASK_PRIO   4
#define DISPLAY_QUEUE_LEN   10

/* ====== Touch task ====== */
#define TOUCH_TASK_STACK    3072
#define TOUCH_TASK_PRIO      3
#define TOUCH_POLL_MS        50
#define TOUCH_DEBOUNCE_MS    300
#define TOUCH_LONG_PRESS_MS  600   /* giữ tay lâu hơn mốc này trên 1 card alarm -> xóa */