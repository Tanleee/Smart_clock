#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "buzzer.h"

#define BUZZER_GPIO       GPIO_NUM_1
#define BUZZER_BEEP_MS    300   /* thời gian kêu mỗi nhịp */
#define BUZZER_PAUSE_MS   300   /* thời gian nghỉ giữa 2 nhịp */

static volatile bool s_ringing = false;

/* Task riêng, chỉ lo bật/tắt GPIO theo nhịp beep-pause khi s_ringing = true.
 * Tách task để việc tạo nhịp kêu không làm nghẽn display_task/touch_task. */
static void buzzer_task(void *pv)
{
    while (1) {
        if (s_ringing) {
            gpio_set_level(BUZZER_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(BUZZER_BEEP_MS));
            if (!s_ringing) continue;   /* vừa bị tắt giữa nhịp -> bỏ qua đoạn nghỉ, vòng lại kiểm tra ngay */
            gpio_set_level(BUZZER_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(BUZZER_PAUSE_MS));
        } else {
            gpio_set_level(BUZZER_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

esp_err_t buzzer_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) return err;
    gpio_set_level(BUZZER_GPIO, 0);

    return (xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 3, NULL) == pdPASS)
           ? ESP_OK : ESP_FAIL;
}

void buzzer_start(void) { s_ringing = true; }
void buzzer_stop(void)  { s_ringing = false; gpio_set_level(BUZZER_GPIO, 0); }