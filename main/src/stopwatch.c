#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display.h"
#include "stopwatch.h"

static volatile bool     s_running      = false;
static volatile uint32_t s_elapsed_sec  = 0;

static void stopwatch_task(void *pv)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (s_running) {
            s_elapsed_sec++;
            display_send(DISPLAY_SRC_STOPWATCH, "TICK");
        }
    }
}

esp_err_t stopwatch_task_start(void)
{
    return (xTaskCreate(stopwatch_task, "stopwatch_task", 2048, NULL, 3, NULL) == pdPASS)
           ? ESP_OK : ESP_FAIL;
}

bool stopwatch_toggle(void)
{
    s_running = !s_running;
    return s_running;
}

void stopwatch_reset(void)
{
    s_running     = false;
    s_elapsed_sec = 0;
}

bool     stopwatch_is_running(void)   { return s_running; }
uint32_t stopwatch_get_elapsed(void)  { return s_elapsed_sec; }