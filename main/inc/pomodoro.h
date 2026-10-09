#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    POMO_FIELD_FOCUS = 0,
    POMO_FIELD_BREAK,
    POMO_FIELD_LONG_BREAK,
    POMO_FIELD_REPS,
    POMO_FIELD_COUNT
} pomodoro_field_t;

typedef enum {
    POMO_PHASE_FOCUS = 0,
    POMO_PHASE_BREAK,
    POMO_PHASE_LONG_BREAK,
} pomodoro_phase_t;

esp_err_t pomodoro_task_start(void);

/* Màn Setup: chỉnh thông số (phút cho 3 field đầu, số lần cho REPS) */
void pomodoro_cfg_adjust(pomodoro_field_t field, int delta);
int  pomodoro_cfg_get(pomodoro_field_t field);

/* Nút Start -> bắt đầu phiên mới từ rep 1, phase FOCUS */
void pomodoro_start(void);

/* Pause/Resume. Trả về true nếu SAU lệnh này đang chạy */
bool pomodoro_toggle(void);

/* Dừng hẳn phiên đang chạy (dùng khi bấm Skip để quay lại Setup) */
void pomodoro_stop(void);

bool             pomodoro_is_running(void);
pomodoro_phase_t pomodoro_get_phase(void);
int              pomodoro_get_rep(void);           /* rep hiện tại, 1-based */
uint32_t         pomodoro_get_remaining_sec(void);

#ifdef __cplusplus
}
#endif