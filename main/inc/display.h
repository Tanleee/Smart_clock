#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Nguồn của chuỗi hiển thị */
typedef enum {
    DISPLAY_SRC_SYSTEM = 0,
    DISPLAY_SRC_WIFI,
    DISPLAY_SRC_SNTP,
    DISPLAY_SRC_TIME,
    DISPLAY_SRC_DATE,
    DISPLAY_SRC_WEEKDAY,
    DISPLAY_SRC_SCREEN,
    DISPLAY_SRC_ALARM_EDIT,  
    DISPLAY_SRC_ALARM_SAVE,  
    DISPLAY_SRC_MAX
} display_source_t;

#define DISPLAY_TEXT_MAX_LEN 64

typedef struct {
    char             text[DISPLAY_TEXT_MAX_LEN];
    display_source_t source;
} display_msg_t;

esp_err_t display_init(void);

void display_send(display_source_t src, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#ifdef __cplusplus
}
#endif