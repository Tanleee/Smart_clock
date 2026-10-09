#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tạo task tick 1 giây/lần (gửi DISPLAY_SRC_STOPWATCH "TICK" khi đang chạy) */
esp_err_t stopwatch_task_start(void);

/* Đảo trạng thái chạy/dừng. Trả về true nếu SAU lệnh này đang chạy. */
bool stopwatch_toggle(void);

/* Đưa về 0 và dừng lại */
void stopwatch_reset(void);

bool     stopwatch_is_running(void);
uint32_t stopwatch_get_elapsed(void);   /* số giây đã trôi qua */

#ifdef __cplusplus
}
#endif