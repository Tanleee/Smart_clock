#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display.h"
#include "alarm_data.h"
#include "alarm_task.h"

#define ALARM_CHECK_PERIOD_MS   1000

static void alarm_task(void *pv)
{
    int last_h = -1, last_m = -1;

    while (1) {
        time_t now;
        struct tm t;
        time(&now);
        localtime_r(&now, &t);

        /* chưa đồng bộ SNTP -> giờ hệ thống chưa đáng tin, bỏ qua */
        if (t.tm_year < (2024 - 1900)) {
            vTaskDelay(pdMS_TO_TICKS(ALARM_CHECK_PERIOD_MS));
            continue;
        }

        /* chỉ kiểm tra khi SANG phút mới, tránh bắn lại nhiều lần trong
         * cùng 1 phút (task này tick mỗi giây) */
        if (t.tm_min != last_m || t.tm_hour != last_h) {
            last_h = t.tm_hour;
            last_m = t.tm_min;

            /* struct tm: tm_wday 0=CN...6=Thứ7
             * alarm_t.days[]:   0=Mon...6=Sun (xem comment trong alarm_data.h)
             * -> đổi hệ: index = (tm_wday + 6) % 7 */
            int wday = (t.tm_wday + 6) % 7;

            int count = alarm_data_count();
            for (int i = 0; i < count; i++) {
                alarm_t *a = alarm_data_get(i);
                if (a && a->enabled && a->hour == t.tm_hour &&
                    a->minute == t.tm_min && a->days[wday]) {
                    display_send(DISPLAY_SRC_ALARM_EDIT, "RING_%02d:%02d", a->hour, a->minute);
                    break;   /* 1 báo thức trùng giờ là đủ, bỏ qua báo thức khác trùng cùng phút */
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(ALARM_CHECK_PERIOD_MS));
    }
}

esp_err_t alarm_task_start(void)
{
    return (xTaskCreate(alarm_task, "alarm_task", 3072, NULL, 4, NULL) == pdPASS)
           ? ESP_OK : ESP_FAIL;
}