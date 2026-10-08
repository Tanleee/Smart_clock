#include <cstdio>
#include <cstdarg>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "app_config.h"
#include "display.h"
#include "ui_layout.h"
#include "LGFX_Config.hpp"

#include "images/dashboard.h"
#include "images/menu.h"
#include "images/alarm.h"
#include "images/stopwatch.h"
#include "images/pomodoro.h"
#include "images/timeset.h"

static const char *TAG         = "smart_clock";
static const char *TAG_DISPLAY = "display";

static QueueHandle_t s_display_queue = NULL;
static LGFX          lcd;

typedef enum {
    SCREEN_DASHBOARD = 0,
    SCREEN_MENU,
    SCREEN_ALARM,
    SCREEN_STOPWATCH,
    SCREEN_POMODORO,
    SCREEN_TIMESET,
    SCREEN_COUNT
} ui_screen_t;

static ui_screen_t s_current_screen = SCREEN_DASHBOARD;

typedef struct {
    const char     *name;
    int32_t         width;
    int32_t         height;
    const uint16_t *image;
} screen_entry_t;

static const screen_entry_t s_screens[SCREEN_COUNT] = {
    [SCREEN_DASHBOARD] = { "DASHBOARD", DASHBOARD_WIDTH, DASHBOARD_HEIGHT, dashboard },
    [SCREEN_MENU]      = { "MENU",      MENU_WIDTH,      MENU_HEIGHT,      menu      },
    [SCREEN_ALARM]     = { "ALARM",     ALARM_WIDTH,     ALARM_HEIGHT,     alarm_bg  },
    [SCREEN_STOPWATCH] = { "STOPWATCH", STOPWATCH_WIDTH, STOPWATCH_HEIGHT, stopwatch },
    [SCREEN_POMODORO]  = { "POMODORO",  POMODORO_WIDTH,  POMODORO_HEIGHT,  pomodoro  },
    [SCREEN_TIMESET]   = { "TIMESET",   TIMESET_WIDTH,   TIMESET_HEIGHT,   timeset   },
};

static char s_cached_time[8]     = {0};
static char s_cached_date[32]    = {0};
static char s_cached_weekday[16] = {0};

static const char *s_source_names[DISPLAY_SRC_MAX] = {
    [DISPLAY_SRC_SYSTEM]  = "SYSTEM",
    [DISPLAY_SRC_WIFI]    = "WIFI",
    [DISPLAY_SRC_SNTP]    = "SNTP",
    [DISPLAY_SRC_TIME]    = "TIME",
    [DISPLAY_SRC_DATE]    = "DATE",
    [DISPLAY_SRC_WEEKDAY] = "WEEKDAY",
    [DISPLAY_SRC_SCREEN]  = "SCREEN",
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

/* Vẽ 1 vùng chữ: tạo sprite đúng kích thước vùng đó, vẽ lại nền rồi in
 * text đè lên, cuối cùng đẩy nguyên sprite ra màn hình (không chớp/giật).
 * bgPatch == nullptr -> tạm fill đen, dùng khi CHƯA cắt ảnh patch nền
 * riêng cho vùng đó (xem lại hướng dẫn tạo patch ở phần trước). */
 static void ui_draw_text(int32_t x, int32_t y, int32_t w, int32_t h,
                           const char *text,
                           const lgfx::IFont *font, uint16_t color)
 {
     LGFX_Sprite spr(&lcd);
     spr.setColorDepth(16);
     spr.setSwapBytes(true);
     spr.createSprite(w, h);

     /* Lấy nền đúng từ ảnh dashboard gốc, copy từng hàng pixel */
     for (int32_t row = 0; row < h; row++) {
         spr.pushImage(0, row, w, 1, &dashboard[(y + row) * DASHBOARD_WIDTH + x]);
     }

     spr.setFont(font);
     spr.setTextColor(color);
     spr.drawString(text, 0, 0);

     spr.pushSprite(x, y);
     spr.deleteSprite();
 }

static std::uint16_t s_touch_cal[8] = { 3625, 309, 3732, 3759, 403, 273, 457, 3687 };
 
static void display_task(void *pvParameters)
{
    /* Khởi tạo LGFX ngay trong task sẽ dùng nó, tránh tranh chấp SPI
     * giữa nhiều task */
    lcd.init();
    lcd.setRotation(3);
    lcd.fillScreen(TFT_BLACK);
    lcd.setSwapBytes(true);
	lcd.setTouchCalibrate(s_touch_cal);
	
    lcd.pushImage(0, 0, DASHBOARD_WIDTH, DASHBOARD_HEIGHT, dashboard);   /* vẽ nền 1 lần duy nhất */

    display_msg_t msg;

    while (1) {
        if (xQueueReceive(s_display_queue, &msg, portMAX_DELAY) == pdTRUE) {
            const char *name = (msg.source < DISPLAY_SRC_MAX) ? s_source_names[msg.source] : "UNKNOWN";
            ESP_LOGI(TAG_DISPLAY, "[%s] %s", name, msg.text);   /* vẫn log song song để debug */

			switch (msg.source) {
				case DISPLAY_SRC_TIME:
				    strncpy(s_cached_time, msg.text, sizeof(s_cached_time) - 1);
				    if (s_current_screen == SCREEN_DASHBOARD)
				        ui_draw_text(UI_CLOCK_X, UI_CLOCK_Y, UI_CLOCK_W, UI_CLOCK_H,
				                     msg.text, &fonts::Font7, TFT_CYAN);
				    break;
	
				case DISPLAY_SRC_DATE:
				    strncpy(s_cached_date, msg.text, sizeof(s_cached_date) - 1);
				    if (s_current_screen == SCREEN_DASHBOARD)
				        ui_draw_text(UI_DATE_X, UI_DATE_Y, UI_DATE_W, UI_DATE_H,
				                     msg.text, &fonts::Font0, TFT_WHITE);
				    break;
	
				case DISPLAY_SRC_WEEKDAY:
				    strncpy(s_cached_weekday, msg.text, sizeof(s_cached_weekday) - 1);
				    if (s_current_screen == SCREEN_DASHBOARD)
				        ui_draw_text(UI_WEEKDAY_X, UI_WEEKDAY_Y, UI_WEEKDAY_W, UI_WEEKDAY_H,
				                     msg.text, &fonts::Font0, TFT_WHITE);
				    break;
	
				case DISPLAY_SRC_SCREEN:
				    for (int i = 0; i < SCREEN_COUNT; i++) {
				        if (strcmp(msg.text, s_screens[i].name) == 0 && s_current_screen != (ui_screen_t)i) {
				            s_current_screen = (ui_screen_t)i;
				            lcd.pushImage(0, 0, s_screens[i].width, s_screens[i].height, s_screens[i].image);

				            if (s_current_screen == SCREEN_DASHBOARD) {
				                /* dashboard cần vẽ lại chữ giờ/ngày/thứ vì ảnh nền đè hết lên */
				                if (s_cached_time[0])
				                    ui_draw_text(UI_CLOCK_X, UI_CLOCK_Y, UI_CLOCK_W, UI_CLOCK_H,
				                                 s_cached_time, &fonts::Font7, TFT_CYAN);
				                if (s_cached_date[0])
				                    ui_draw_text(UI_DATE_X, UI_DATE_Y, UI_DATE_W, UI_DATE_H,
				                                 s_cached_date, &fonts::Font0, TFT_WHITE);
				                if (s_cached_weekday[0])
				                    ui_draw_text(UI_WEEKDAY_X, UI_WEEKDAY_Y, UI_WEEKDAY_W, UI_WEEKDAY_H,
				                                 s_cached_weekday, &fonts::Font0, TFT_WHITE);
				            }
				            break;
				        }
				    }
				    break;
	
				case DISPLAY_SRC_WIFI:
				case DISPLAY_SRC_SNTP:
				case DISPLAY_SRC_SYSTEM:
				default:
				    break;
			}
        }
    }
}

static void touch_task(void *pvParameters)
{
    bool was_pressed = false;
    TickType_t last_trigger = 0;

#define IN_RECT(px, py, rx, ry, rw, rh) \
    ((px) >= (rx) && (px) <= (rx)+(rw) && (py) >= (ry) && (py) <= (ry)+(rh))

    while (1) {
        int32_t tx, ty;
        bool pressed = lcd.getTouch(&tx, &ty);

        if (pressed && !was_pressed) {
            TickType_t now = xTaskGetTickCount();
            if ((now - last_trigger) > pdMS_TO_TICKS(TOUCH_DEBOUNCE_MS)) {

                switch (s_current_screen) {
                case SCREEN_DASHBOARD:
                    if (IN_RECT(tx, ty, UI_MENU_BTN_X, UI_MENU_BTN_Y, UI_MENU_BTN_W, UI_MENU_BTN_H))
                        display_send(DISPLAY_SRC_SCREEN, "MENU");
                    break;

                case SCREEN_MENU:
                    if (IN_RECT(tx, ty, UI_BACK_BTN_X, UI_BACK_BTN_Y, UI_BACK_BTN_W, UI_BACK_BTN_H))
                        display_send(DISPLAY_SRC_SCREEN, "DASHBOARD");
                    else if (IN_RECT(tx, ty, UI_MENU_ALARM_X, UI_MENU_ALARM_Y, UI_MENU_ALARM_W, UI_MENU_ALARM_H))
                        display_send(DISPLAY_SRC_SCREEN, "ALARM");
                    else if (IN_RECT(tx, ty, UI_MENU_STOPWATCH_X, UI_MENU_STOPWATCH_Y, UI_MENU_STOPWATCH_W, UI_MENU_STOPWATCH_H))
                        display_send(DISPLAY_SRC_SCREEN, "STOPWATCH");
                    else if (IN_RECT(tx, ty, UI_MENU_POMODORO_X, UI_MENU_POMODORO_Y, UI_MENU_POMODORO_W, UI_MENU_POMODORO_H))
                        display_send(DISPLAY_SRC_SCREEN, "POMODORO");
                    else if (IN_RECT(tx, ty, UI_MENU_SETTIME_X, UI_MENU_SETTIME_Y, UI_MENU_SETTIME_W, UI_MENU_SETTIME_H))
                        display_send(DISPLAY_SRC_SCREEN, "TIMESET");
                    break;

                case SCREEN_ALARM:
                case SCREEN_STOPWATCH:
                case SCREEN_POMODORO:
                case SCREEN_TIMESET:
                    if (IN_RECT(tx, ty, UI_BACK_BTN_X, UI_BACK_BTN_Y, UI_BACK_BTN_W, UI_BACK_BTN_H))
                        display_send(DISPLAY_SRC_SCREEN, "MENU");
                    break;

                default:
                    break;
                }

                last_trigger = now;
            }
        }
        was_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(TOUCH_POLL_MS));
    }

#undef IN_RECT
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
	
	if (xTaskCreate(touch_task, "touch_task", TOUCH_TASK_STACK,
	                NULL, TOUCH_TASK_PRIO, NULL) != pdPASS) {
	    return ESP_FAIL;
	}
	
    return ESP_OK;
}