#pragma once

#include "esp_err.h"

/* Nguồn của chuỗi hiển thị */
typedef enum {
    DISPLAY_SRC_SYSTEM = 0,   /* thông báo hệ thống */
    DISPLAY_SRC_WIFI,         /* trạng thái WiFi */
    DISPLAY_SRC_SNTP,         /* trạng thái đồng bộ giờ */
    DISPLAY_SRC_TIME,         /* giờ hiện tại */
    DISPLAY_SRC_MAX
} display_source_t;

#define DISPLAY_TEXT_MAX_LEN 64

/* Phần tử được đưa vào queue */
typedef struct {
    char             text[DISPLAY_TEXT_MAX_LEN];  /* chuỗi cần hiển thị */
    display_source_t source;                      /* nguồn của chuỗi */
} display_msg_t;

/* Tạo queue và task display. Gọi một lần trong app_main trước các module khác */
esp_err_t display_init(void);

/* Gửi một chuỗi vào queue (không chặn). Dùng như printf */
void display_send(display_source_t src, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
