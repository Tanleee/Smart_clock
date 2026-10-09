#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display.h"
#include "pomodoro.h"

#define POMO_FOCUS_DEFAULT        25
#define POMO_BREAK_DEFAULT         5
#define POMO_LONG_BREAK_DEFAULT   15
#define POMO_REPS_DEFAULT          4

#define POMO_FOCUS_MAX        60
#define POMO_BREAK_MAX         30
#define POMO_LONG_BREAK_MAX    60
#define POMO_REPS_MAX          10

static int s_cfg[POMO_FIELD_COUNT] = {
    POMO_FOCUS_DEFAULT, POMO_BREAK_DEFAULT, POMO_LONG_BREAK_DEFAULT, POMO_REPS_DEFAULT
};

static volatile bool             s_running       = false;
static volatile pomodoro_phase_t s_phase         = POMO_PHASE_FOCUS;
static volatile int              s_rep           = 1;
static volatile uint32_t         s_remaining_sec = 0;

static uint32_t phase_duration_sec(pomodoro_phase_t phase)
{
    switch (phase) {
        case POMO_PHASE_FOCUS:      return (uint32_t)s_cfg[POMO_FIELD_FOCUS] * 60;
        case POMO_PHASE_BREAK:      return (uint32_t)s_cfg[POMO_FIELD_BREAK] * 60;
        case POMO_PHASE_LONG_BREAK: return (uint32_t)s_cfg[POMO_FIELD_LONG_BREAK] * 60;
    }
    return 0;
}

/* Chuyển sang phase/rep kế tiếp khi đồng hồ về 0.
 * Trả về false nếu vừa xong long break cuối -> hoàn thành cả phiên. */
static bool advance_phase(void)
{
    if (s_phase == POMO_PHASE_FOCUS) {
        s_phase = (s_rep >= s_cfg[POMO_FIELD_REPS]) ? POMO_PHASE_LONG_BREAK : POMO_PHASE_BREAK;
    } else if (s_phase == POMO_PHASE_BREAK) {
        s_rep++;
        s_phase = POMO_PHASE_FOCUS;
    } else {
        return false;   /* vừa xong long break -> hết phiên */
    }
    s_remaining_sec = phase_duration_sec(s_phase);
    return true;
}

static void pomodoro_task(void *pv)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (!s_running) continue;

        if (s_remaining_sec > 0) {
            s_remaining_sec--;
            display_send(DISPLAY_SRC_POMODORO, "TICK");
        } else if (advance_phase()) {
            display_send(DISPLAY_SRC_POMODORO, "PHASE");
        } else {
            s_running = false;
            display_send(DISPLAY_SRC_POMODORO, "DONE");
        }
    }
}

esp_err_t pomodoro_task_start(void)
{
    return (xTaskCreate(pomodoro_task, "pomodoro_task", 2048, NULL, 3, NULL) == pdPASS)
           ? ESP_OK : ESP_FAIL;
}

void pomodoro_cfg_adjust(pomodoro_field_t field, int delta)
{
    if (field >= POMO_FIELD_COUNT) return;
    int hi = (field == POMO_FIELD_FOCUS)      ? POMO_FOCUS_MAX
           : (field == POMO_FIELD_BREAK)      ? POMO_BREAK_MAX
           : (field == POMO_FIELD_LONG_BREAK) ? POMO_LONG_BREAK_MAX
                                               : POMO_REPS_MAX;
    int v = s_cfg[field] + delta;
    if (v < 1)  v = 1;
    if (v > hi) v = hi;
    s_cfg[field] = v;
}

int pomodoro_cfg_get(pomodoro_field_t field)
{
    return (field < POMO_FIELD_COUNT) ? s_cfg[field] : 0;
}

void pomodoro_start(void)
{
    s_phase         = POMO_PHASE_FOCUS;
    s_rep           = 1;
    s_remaining_sec = phase_duration_sec(POMO_PHASE_FOCUS);
    s_running       = true;
}

bool pomodoro_toggle(void) { s_running = !s_running; return s_running; }
void pomodoro_stop(void)   { s_running = false; }

bool             pomodoro_is_running(void)        { return s_running; }
pomodoro_phase_t pomodoro_get_phase(void)         { return s_phase; }
int              pomodoro_get_rep(void)           { return s_rep; }
uint32_t         pomodoro_get_remaining_sec(void) { return s_remaining_sec; }