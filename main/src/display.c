#include <stdio.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "app_config.h"
#include "display.h"

static const char *TAG         = "smart_clock";
static const char *TAG_DISPLAY = "display";

static QueueHandle_t s_display_queue = NULL;

static const char *s_source_names[DISPLAY_SRC_MAX] = {
    [DISPLAY_SRC_SYSTEM] = "SYSTEM",
    [DISPLAY_SRC_WIFI]   = "WIFI",
    [DISPLAY_SRC_SNTP]   = "SNTP",
    [DISPLAY_SRC_TIME]   = "TIME",
};

void display_send(display_source_t src, const char *fmt, ...)
{
    if (s_display_queue == NULL) {
        return;
    }

    display_msg_t msg;
    msg.source = src;

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg.text, sizeof(msg.text), fmt, args);
    va_end(args);

    if (xQueueSend(s_display_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Display queue full, drop message from %s", s_source_names[src]);
    }
}

static void display_task(void *pvParameters)
{
    display_msg_t msg;

    while (1) {
        if (xQueueReceive(s_display_queue, &msg, portMAX_DELAY) == pdTRUE) {
            const char *name = (msg.source < DISPLAY_SRC_MAX) ? s_source_names[msg.source] : "UNKNOWN";

            /* Hiện tại in ra terminal. Sau này thay phần này bằng code vẽ lên TFT */
            switch (msg.source) {
            case DISPLAY_SRC_TIME:
                ESP_LOGI(TAG_DISPLAY, "[%s] %s", name, msg.text);
                break;
            case DISPLAY_SRC_WIFI:
            case DISPLAY_SRC_SNTP:
            case DISPLAY_SRC_SYSTEM:
            default:
                ESP_LOGI(TAG_DISPLAY, "[%s] %s", name, msg.text);
                break;
            }
        }
    }
}

esp_err_t display_init(void)
{
    s_display_queue = xQueueCreate(DISPLAY_QUEUE_LEN, sizeof(display_msg_t));
    if (s_display_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(display_task, "display_task", DISPLAY_TASK_STACK,
                    NULL, DISPLAY_TASK_PRIO, NULL) != pdPASS) {
        return ESP_FAIL;
    }
    return ESP_OK;
}
