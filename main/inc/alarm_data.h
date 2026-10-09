#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ALARM_MAX_COUNT 10

typedef struct {
    uint8_t hour;      /* 0-23, lưu dạng 24h */
    uint8_t minute;    /* 0-59 */
    bool    days[7];   /* 0=Mon...6=Sun, khớp thứ tự hiển thị M T W T F S S */
    bool    enabled;
    bool    valid;     /* false = ô trống */
} alarm_t;

esp_err_t alarm_data_init(void);
int       alarm_data_count(void);
alarm_t  *alarm_data_get(int index);
esp_err_t alarm_data_add(const alarm_t *a);
esp_err_t alarm_data_update(int index, const alarm_t *a);
esp_err_t alarm_data_remove(int index);
esp_err_t alarm_data_set_enabled(int index, bool enabled);

#ifdef __cplusplus
}
#endif