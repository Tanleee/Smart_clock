#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "app_config.h"
#include "LGFX_Config.hpp"
#include "display.h"
#include "ui_layout.h"
#include "alarm_data.h"
#include "buzzer.h"

#include "images/dashboard.h"
#include "images/menu.h"
#include "images/alarm.h"
#include "images/stopwatch.h"
#include "images/pomodoro.h"
#include "images/timeset.h"
#include "images/add_alarm.h"

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
    SCREEN_ADD_ALARM,   
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
    [SCREEN_ADD_ALARM] = { "ADDALARM",  ADDALARM_WIDTH,  ADDALARM_HEIGHT,  add_alarm_bg },
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
	[DISPLAY_SRC_ALARM_EDIT] = "ALARM_EDIT",
	[DISPLAY_SRC_ALARM_SAVE] = "ALARM_SAVE",
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
                           const char *text, const lgfx::IFont *font, uint16_t color,
                           const uint16_t *bg_image, int32_t bg_width)   /* 2 tham số mới */
 {
     LGFX_Sprite spr(&lcd);
     spr.setColorDepth(16);
     spr.setSwapBytes(true);
     spr.createSprite(w, h);

     for (int32_t row = 0; row < h; row++) {
         spr.pushImage(0, row, w, 1, &bg_image[(y + row) * bg_width + x]);
     }

     spr.setFont(font);
     spr.setTextColor(color);
     spr.drawString(text, 0, 0);

     spr.pushSprite(x, y);
     spr.deleteSprite();
 }

 /* Khôi phục nguyên 1 vùng chữ nhật từ ảnh nền gốc (dùng trước khi vẽ lại
  * card báo thức, để không để lại "vết" của lần vẽ trước). Cùng kỹ thuật
  * sprite theo từng hàng như ui_draw_text ở trên. */
 static void ui_restore_bg_region(int32_t x, int32_t y, int32_t w, int32_t h,
                                   const uint16_t *bg_image, int32_t bg_width)
 {
     LGFX_Sprite spr(&lcd);
     spr.setColorDepth(16);
     spr.setSwapBytes(true);
     spr.createSprite(w, h);

     for (int32_t row = 0; row < h; row++) {
         spr.pushImage(0, row, w, 1, &bg_image[(y + row) * bg_width + x]);
     }

     spr.pushSprite(x, y);
     spr.deleteSprite();
 }

 static const char s_day_letters[7] = {'M','T','W','T','F','S','S'};
 static int  s_draft_hour12 = 7;
 static int  s_draft_minute = 0;
 static bool s_draft_is_pm  = false;
 static bool s_draft_days[7] = {0};
 /* -1 = đang tạo báo thức mới; >=0 = đang sửa báo thức có sẵn, giá trị là
  * chỉ số (theo alarm_data_get) của báo thức đó */
 static int  s_edit_index = -1;
 /* true khi đang hiển thị khung báo thức reo giữa màn hình */
 static bool s_ringing = false;
 
 static void ui_draw_addalarm_digit(int32_t x, int32_t y, int32_t w, int32_t h, int value)
 {
     char buf[4];
     snprintf(buf, sizeof(buf), "%02d", value);
     ui_draw_text(x, y, w, h, buf, &fonts::Font7, TFT_CYAN, add_alarm_bg, ADDALARM_WIDTH);
 }

 static void ui_draw_ampm(bool is_pm)
 {
     ui_draw_text(UI_ADDALARM_AMPM_X, UI_ADDALARM_AMPM_Y, UI_ADDALARM_AMPM_W, UI_ADDALARM_AMPM_H,
                  is_pm ? "PM" : "AM", &fonts::Font0, TFT_CYAN, add_alarm_bg, ADDALARM_WIDTH);
 }

 static void ui_draw_day_circle_at(int index, bool selected)
 {
     int32_t cx = UI_ADDALARM_DAY0_CX + index * UI_ADDALARM_DAY_STEP;
     int32_t cy = UI_ADDALARM_DAY_CY;
     int32_t r  = UI_ADDALARM_DAY_R;

     LGFX_Sprite spr(&lcd);
     spr.setColorDepth(16);
     spr.setSwapBytes(true);
     int32_t size = r * 2 + 2;
     spr.createSprite(size, size);

     for (int32_t row = 0; row < size; row++) {
         spr.pushImage(0, row, size, 1,
                       &add_alarm_bg[(cy - r - 1 + row) * ADDALARM_WIDTH + (cx - r - 1)]);
     }

     if (selected) {
         spr.fillCircle(r + 1, r + 1, r, TFT_CYAN);
         spr.setTextColor(TFT_BLACK);
     } else {
         spr.drawCircle(r + 1, r + 1, r, TFT_CYAN);
         spr.setTextColor(TFT_CYAN);
     }

     spr.setFont(&fonts::Font0);
     char letter[2] = { s_day_letters[index], 0 };
     spr.drawString(letter, r + 1 - 3, r + 1 - 4);

     spr.pushSprite(cx - r - 1, cy - r - 1);
     spr.deleteSprite();
 } 

 /* ====== Danh sách báo thức (màn SCREEN_ALARM) ======
  * Mỗi báo thức hợp lệ được bọc trong 1 khung round-rectangle. Chạm ngắn
  * vào card -> mở New Alarm với dữ liệu cũ để sửa. Giữ tay lâu hơn
  * TOUCH_LONG_PRESS_MS trên 1 card -> xóa báo thức đó (xử lý giữ/nhả nằm
  * bên touch_task, hàm dưới đây chỉ lo phần vẽ). */

 static void ui_draw_alarm_card(int slot, const alarm_t *a)
 {
     int32_t x = UI_ALARMLIST_X;
     int32_t y = UI_ALARMLIST_Y0 + slot * (UI_ALARMLIST_H + UI_ALARMLIST_GAP);
     int32_t w = UI_ALARMLIST_W;
     int32_t h = UI_ALARMLIST_H;
     const int32_t radius  = 10;
     const uint16_t card_bg = lcd.color565(8, 16, 36);   /* nền navy tối, đồng tông với theme */

     /* xóa vết card cũ (nếu có) bằng cách khôi phục đúng vùng đó từ ảnh nền */
     ui_restore_bg_region(x, y, w, h, alarm_bg, ALARM_WIDTH);

     lcd.fillRoundRect(x, y, w, h, radius, card_bg);
     lcd.drawRoundRect(x, y, w, h, radius, a->enabled ? TFT_CYAN : TFT_DARKGREY);

     char time_buf[8];
     int hour12 = a->hour % 12;
     if (hour12 == 0) hour12 = 12;
     snprintf(time_buf, sizeof(time_buf), "%02d:%02d", hour12, a->minute);

     lcd.setFont(&fonts::Font4);
     lcd.setTextColor(a->enabled ? TFT_CYAN : TFT_DARKGREY, card_bg);
     lcd.drawString(time_buf, x + 14, y + (h - 26) / 2);

     lcd.setFont(&fonts::Font0);
     lcd.drawString(a->hour < 12 ? "AM" : "PM", x + 92, y + (h - 8) / 2);

     /* 7 vòng tròn M T W T F S S, sáng = ngày được bật */
     for (int d = 0; d < 7; d++) {
         int32_t cx = x + 150 + d * 19;
         int32_t cy = y + h / 2;
         const int32_t r = 7;
         if (a->days[d]) {
             lcd.fillCircle(cx, cy, r, TFT_CYAN);
             lcd.setTextColor(TFT_BLACK, TFT_CYAN);
         } else {
             lcd.drawCircle(cx, cy, r, TFT_DARKGREY);
             lcd.setTextColor(TFT_DARKGREY, card_bg);
         }
         char letter[2] = { s_day_letters[d], 0 };
         lcd.drawString(letter, cx - 3, cy - 4);
     }
 }

 static void ui_draw_alarm_list(void)
 {
     int count = alarm_data_count();
     for (int i = 0; i < UI_ALARMLIST_VISIBLE; i++) {
         if (i < count) {
             alarm_t *a = alarm_data_get(i);
             if (a) ui_draw_alarm_card(i, a);
         } else {
             /* ô trống -> chỉ khôi phục nền, không vẽ card */
             int32_t y = UI_ALARMLIST_Y0 + i * (UI_ALARMLIST_H + UI_ALARMLIST_GAP);
             ui_restore_bg_region(UI_ALARMLIST_X, y, UI_ALARMLIST_W, UI_ALARMLIST_H,
                                  alarm_bg, ALARM_WIDTH);
         }
     }
     /* Lưu ý: ALARM_MAX_COUNT = 10 nhưng màn chỉ hiển thị vừa UI_ALARMLIST_VISIBLE (3)
      * báo thức không cuộn. Nếu cần hỗ trợ nhiều hơn, sẽ phải thêm cơ chế
      * cuộn/phân trang — hiện tại các báo thức dư ra sẽ không hiển thị. */
 }

 static void ui_draw_alarm_ring_overlay(int hour24, int minute)
 {
     const int32_t x = UI_RING_CARD_X, y = UI_RING_CARD_Y;
     const int32_t w = UI_RING_CARD_W, h = UI_RING_CARD_H;
     const int32_t radius = 14;
     const uint16_t card_bg = lcd.color565(10, 14, 30);

     lcd.fillRoundRect(x, y, w, h, radius, card_bg);
     lcd.drawRoundRect(x, y, w, h, radius, TFT_CYAN);

     lcd.setFont(&fonts::Font0);
     lcd.setTextColor(TFT_WHITE, card_bg);
     lcd.drawString("ALARM", x + 16, y + 10);
     lcd.drawString(hour24 < 12 ? "AM" : "PM", x + w - 36, y + 10);

     int hour12 = hour24 % 12;
     if (hour12 == 0) hour12 = 12;
     char time_buf[8];
     snprintf(time_buf, sizeof(time_buf), "%02d:%02d", hour12, minute);

     lcd.setFont(&fonts::Font7);
     lcd.setTextColor(TFT_CYAN, card_bg);
     lcd.drawString(time_buf, x + 44, y + 26);

     lcd.fillRoundRect(UI_RING_BTN_X, UI_RING_BTN_Y, UI_RING_BTN_W, UI_RING_BTN_H, 8, TFT_CYAN);
     lcd.setFont(&fonts::Font0);
     lcd.setTextColor(TFT_BLACK, TFT_CYAN);
     lcd.drawString("DISMISS", UI_RING_BTN_X + 18, UI_RING_BTN_Y + 7);
 }

 /* Xoá overlay bằng cách vẽ lại đúng màn hình hiện tại ở dưới nó */
 static void ui_clear_alarm_ring_overlay(void)
 {
     const screen_entry_t *scr = &s_screens[s_current_screen];
     lcd.pushImage(0, 0, scr->width, scr->height, scr->image);

     if (s_current_screen == SCREEN_DASHBOARD) {
         if (s_cached_time[0])
             ui_draw_text(UI_CLOCK_X, UI_CLOCK_Y, UI_CLOCK_W, UI_CLOCK_H,
                          s_cached_time, &fonts::Font7, TFT_CYAN, dashboard, DASHBOARD_WIDTH);
         if (s_cached_date[0])
             ui_draw_text(UI_DATE_X, UI_DATE_Y, UI_DATE_W, UI_DATE_H,
                          s_cached_date, &fonts::Font0, TFT_WHITE, dashboard, DASHBOARD_WIDTH);
         if (s_cached_weekday[0])
             ui_draw_text(UI_WEEKDAY_X, UI_WEEKDAY_Y, UI_WEEKDAY_W, UI_WEEKDAY_H,
                          s_cached_weekday, &fonts::Font0, TFT_WHITE, dashboard, DASHBOARD_WIDTH);
     } else if (s_current_screen == SCREEN_ALARM) {
         ui_draw_alarm_list();
     } else if (s_current_screen == SCREEN_ADD_ALARM) {
         ui_draw_addalarm_digit(UI_ADDALARM_HOUR_DIGIT_X, UI_ADDALARM_HOUR_DIGIT_Y,
                                 UI_ADDALARM_HOUR_DIGIT_W, UI_ADDALARM_HOUR_DIGIT_H, s_draft_hour12);
         ui_draw_addalarm_digit(UI_ADDALARM_MIN_DIGIT_X, UI_ADDALARM_MIN_DIGIT_Y,
                                 UI_ADDALARM_MIN_DIGIT_W, UI_ADDALARM_MIN_DIGIT_H, s_draft_minute);
         ui_draw_ampm(s_draft_is_pm);
         for (int d = 0; d < 7; d++) ui_draw_day_circle_at(d, s_draft_days[d]);
     }
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
				                     msg.text, &fonts::Font7, TFT_CYAN, dashboard, DASHBOARD_WIDTH);
				    break;
	
				case DISPLAY_SRC_DATE:
				    strncpy(s_cached_date, msg.text, sizeof(s_cached_date) - 1);
				    if (s_current_screen == SCREEN_DASHBOARD)
				        ui_draw_text(UI_DATE_X, UI_DATE_Y, UI_DATE_W, UI_DATE_H,
				                     msg.text, &fonts::Font0, TFT_WHITE, dashboard, DASHBOARD_WIDTH);
				    break;
	
				case DISPLAY_SRC_WEEKDAY:
				    strncpy(s_cached_weekday, msg.text, sizeof(s_cached_weekday) - 1);
				    if (s_current_screen == SCREEN_DASHBOARD)
				        ui_draw_text(UI_WEEKDAY_X, UI_WEEKDAY_Y, UI_WEEKDAY_W, UI_WEEKDAY_H,
				                     msg.text, &fonts::Font0, TFT_WHITE, dashboard, DASHBOARD_WIDTH);
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
				                                 s_cached_time, &fonts::Font7, TFT_CYAN, dashboard, DASHBOARD_WIDTH);
				                if (s_cached_date[0])
				                    ui_draw_text(UI_DATE_X, UI_DATE_Y, UI_DATE_W, UI_DATE_H,
				                                 s_cached_date, &fonts::Font0, TFT_WHITE, dashboard, DASHBOARD_WIDTH);
				                if (s_cached_weekday[0])
				                    ui_draw_text(UI_WEEKDAY_X, UI_WEEKDAY_Y, UI_WEEKDAY_W, UI_WEEKDAY_H,
				                                 s_cached_weekday, &fonts::Font0, TFT_WHITE, dashboard, DASHBOARD_WIDTH);
				            }
							else if (s_current_screen == SCREEN_ALARM) {
								/* vẽ danh sách báo thức hiện có mỗi lần vào màn này
								 * (sau khi thêm/sửa/xóa đều quay lại đây nên luôn cập nhật) */
								ui_draw_alarm_list();
							}
							else if (s_current_screen == SCREEN_ADD_ALARM) {
							    if (s_edit_index < 0) {
							        /* thêm báo thức mới -> giá trị mặc định */
							        s_draft_hour12 = 7;
							        s_draft_minute = 0;
							        s_draft_is_pm  = false;
							        memset(s_draft_days, 0, sizeof(s_draft_days));
							    }
							    /* nếu s_edit_index >= 0, s_draft_* đã được nạp sẵn từ báo thức
							     * cũ ở bước DISPLAY_SRC_ALARM_EDIT "EDIT_%d" ngay trước đó */

							    ui_draw_addalarm_digit(UI_ADDALARM_HOUR_DIGIT_X, UI_ADDALARM_HOUR_DIGIT_Y,
							                            UI_ADDALARM_HOUR_DIGIT_W, UI_ADDALARM_HOUR_DIGIT_H, s_draft_hour12);
							    ui_draw_addalarm_digit(UI_ADDALARM_MIN_DIGIT_X, UI_ADDALARM_MIN_DIGIT_Y,
							                            UI_ADDALARM_MIN_DIGIT_W, UI_ADDALARM_MIN_DIGIT_H, s_draft_minute);
							    ui_draw_ampm(s_draft_is_pm);
							    for (int d = 0; d < 7; d++) ui_draw_day_circle_at(d, s_draft_days[d]);
							}
							
				            break;
				        }
				    }
				    break;
				
				case DISPLAY_SRC_ALARM_EDIT:
				    if (strcmp(msg.text, "HOUR_UP") == 0) {
				        s_draft_hour12 = (s_draft_hour12 % 12) + 1;
				        ui_draw_addalarm_digit(UI_ADDALARM_HOUR_DIGIT_X, UI_ADDALARM_HOUR_DIGIT_Y,
				                                UI_ADDALARM_HOUR_DIGIT_W, UI_ADDALARM_HOUR_DIGIT_H, s_draft_hour12);
				    } else if (strcmp(msg.text, "HOUR_DOWN") == 0) {
				        s_draft_hour12 = (s_draft_hour12 == 1) ? 12 : s_draft_hour12 - 1;
				        ui_draw_addalarm_digit(UI_ADDALARM_HOUR_DIGIT_X, UI_ADDALARM_HOUR_DIGIT_Y,
				                                UI_ADDALARM_HOUR_DIGIT_W, UI_ADDALARM_HOUR_DIGIT_H, s_draft_hour12);
				    } else if (strcmp(msg.text, "MIN_UP") == 0) {
				        s_draft_minute = (s_draft_minute + 1) % 60;
				        ui_draw_addalarm_digit(UI_ADDALARM_MIN_DIGIT_X, UI_ADDALARM_MIN_DIGIT_Y,
				                                UI_ADDALARM_MIN_DIGIT_W, UI_ADDALARM_MIN_DIGIT_H, s_draft_minute);
				    } else if (strcmp(msg.text, "MIN_DOWN") == 0) {
				        s_draft_minute = (s_draft_minute == 0) ? 59 : s_draft_minute - 1;
				        ui_draw_addalarm_digit(UI_ADDALARM_MIN_DIGIT_X, UI_ADDALARM_MIN_DIGIT_Y,
				                                UI_ADDALARM_MIN_DIGIT_W, UI_ADDALARM_MIN_DIGIT_H, s_draft_minute);
				    } else if (strcmp(msg.text, "AMPM_TOGGLE") == 0) {
				        s_draft_is_pm = !s_draft_is_pm;
				        ui_draw_ampm(s_draft_is_pm);
				    } else if (strncmp(msg.text, "DAY_", 4) == 0) {
				        int idx = atoi(msg.text + 4);
				        if (idx >= 0 && idx < 7) {
				            s_draft_days[idx] = !s_draft_days[idx];
				            ui_draw_day_circle_at(idx, s_draft_days[idx]);
				        }
				    } else if (strcmp(msg.text, "NEW") == 0) {
				        /* nút "+" trên màn Alarm -> tạo mới, không giữ dữ liệu cũ */
				        s_edit_index = -1;
				        display_send(DISPLAY_SRC_SCREEN, "ADDALARM");
				    } else if (strncmp(msg.text, "EDIT_", 5) == 0) {
				        /* chạm ngắn vào 1 card -> nạp dữ liệu cũ rồi mở New Alarm để sửa */
				        int idx = atoi(msg.text + 5);
				        alarm_t *a = alarm_data_get(idx);
				        if (a) {
				            s_edit_index = idx;
				            int h12 = a->hour % 12;
				            if (h12 == 0) h12 = 12;
				            s_draft_hour12 = h12;
				            s_draft_minute = a->minute;
				            s_draft_is_pm  = (a->hour >= 12);
				            memcpy(s_draft_days, a->days, sizeof(s_draft_days));
				            display_send(DISPLAY_SRC_SCREEN, "ADDALARM");
				        }
					} else if (strcmp(msg.text, "RING_STOP") == 0) {
					    if (s_ringing) {
					        s_ringing = false;
					        buzzer_stop();
					        ui_clear_alarm_ring_overlay();
					    }
					} else if (strncmp(msg.text, "RING_", 5) == 0) {
					    if (!s_ringing) {
					        int hh = 0, mm = 0;
					        if (sscanf(msg.text + 5, "%d:%d", &hh, &mm) == 2) {
					            s_ringing = true;
					            buzzer_start();
					            ui_draw_alarm_ring_overlay(hh, mm);
					        }
					    }
					} else if (strncmp(msg.text, "DELETE_", 7) == 0) {
				        /* giữ tay lâu trên 1 card -> xóa báo thức đó rồi vẽ lại danh sách */
				        int idx = atoi(msg.text + 7);
				        if (alarm_data_remove(idx) == ESP_OK) {
				            ESP_LOGI(TAG_DISPLAY, "Alarm %d deleted", idx);
				        } else {
				            ESP_LOGW(TAG_DISPLAY, "Delete failed, index %d not found", idx);
				        }
				        if (s_current_screen == SCREEN_ALARM) ui_draw_alarm_list();
				    }
				    break;

				case DISPLAY_SRC_ALARM_SAVE:
				    {
				        alarm_t a = {};   /* đổi từ {0} sang {} */
				        a.hour = s_draft_is_pm ? ((s_draft_hour12 % 12) + 12) : (s_draft_hour12 % 12);
				        a.minute = s_draft_minute;
				        memcpy(a.days, s_draft_days, sizeof(a.days));
				        a.enabled = true;
				        a.valid = true;

				        esp_err_t err;
				        if (s_edit_index >= 0) {
				            err = alarm_data_update(s_edit_index, &a);
				            if (err == ESP_OK)
				                ESP_LOGI(TAG_DISPLAY, "Alarm %d updated: %02d:%02d", s_edit_index, a.hour, a.minute);
				            else
				                ESP_LOGW(TAG_DISPLAY, "Alarm update failed (index %d)", s_edit_index);
				        } else {
				            err = alarm_data_add(&a);
				            if (err == ESP_OK)
				                ESP_LOGI(TAG_DISPLAY, "Alarm saved: %02d:%02d", a.hour, a.minute);
				            else
				                ESP_LOGW(TAG_DISPLAY, "Alarm list full");
				        }
				        s_edit_index = -1;   /* reset để lần sau bấm "+" không bị dính dữ liệu cũ */
				    }
				    display_send(DISPLAY_SRC_SCREEN, "ALARM");
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

    /* Theo dõi thao tác nhấn-giữ trên 1 card báo thức ở màn SCREEN_ALARM:
     * chạm xuống trong 1 card -> ghi nhận chỉ số + thời điểm bắt đầu, CHƯA
     * gửi lệnh gì. Nếu giữ đủ lâu -> xóa. Nếu nhả tay sớm hơn -> coi là
     * chạm ngắn, mở New Alarm để sửa. */
    int        press_alarm_idx = -1;
    TickType_t press_start     = 0;
    bool       long_fired      = false;

#define IN_RECT(px, py, rx, ry, rw, rh) \
    ((px) >= (rx) && (px) <= (rx)+(rw) && (py) >= (ry) && (py) <= (ry)+(rh))

    while (1) {
        int32_t tx, ty;
        bool pressed = lcd.getTouch(&tx, &ty);
        TickType_t now = xTaskGetTickCount();
		
		if (s_ringing) {
		    if (pressed && !was_pressed &&
		        IN_RECT(tx, ty, UI_RING_BTN_X, UI_RING_BTN_Y, UI_RING_BTN_W, UI_RING_BTN_H)) {
		        display_send(DISPLAY_SRC_ALARM_EDIT, "RING_STOP");
		    }
		    was_pressed = pressed;
		    vTaskDelay(pdMS_TO_TICKS(TOUCH_POLL_MS));
		    continue;   /* bỏ qua toàn bộ logic chạm của các màn khác khi đang reo */
		}
		
        if (pressed && !was_pressed) {
            /* ----- cạnh lên: vừa chạm xuống ----- */
            if ((now - last_trigger) > pdMS_TO_TICKS(TOUCH_DEBOUNCE_MS)) {

                if (s_current_screen == SCREEN_ALARM) {
                    int idx = -1;
                    int count = alarm_data_count();
                    for (int i = 0; i < count && i < UI_ALARMLIST_VISIBLE; i++) {
                        int32_t cy = UI_ALARMLIST_Y0 + i * (UI_ALARMLIST_H + UI_ALARMLIST_GAP);
                        if (IN_RECT(tx, ty, UI_ALARMLIST_X, cy, UI_ALARMLIST_W, UI_ALARMLIST_H)) {
                            idx = i;
                            break;
                        }
                    }

                    if (idx >= 0) {
                        /* chạm vào 1 card -> chỉ bắt đầu đếm giờ, chờ xem nhả sớm hay giữ lâu */
                        press_alarm_idx = idx;
                        press_start     = now;
                        long_fired      = false;
                    } else {
                        press_alarm_idx = -1;
                        if (IN_RECT(tx, ty, UI_BACK_BTN_X, UI_BACK_BTN_Y, UI_BACK_BTN_W, UI_BACK_BTN_H))
                            display_send(DISPLAY_SRC_SCREEN, "MENU");
                        else if (IN_RECT(tx, ty, UI_ALARM_ADD_BTN_X, UI_ALARM_ADD_BTN_Y, UI_ALARM_ADD_BTN_W, UI_ALARM_ADD_BTN_H))
                            display_send(DISPLAY_SRC_ALARM_EDIT, "NEW");
                        last_trigger = now;
                    }

                } else {
                    press_alarm_idx = -1;

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

                    case SCREEN_STOPWATCH:
                    case SCREEN_POMODORO:
                    case SCREEN_TIMESET:
                        if (IN_RECT(tx, ty, UI_BACK_BTN_X, UI_BACK_BTN_Y, UI_BACK_BTN_W, UI_BACK_BTN_H))
                            display_send(DISPLAY_SRC_SCREEN, "MENU");
                        break;

                    case SCREEN_ADD_ALARM:
                        if (IN_RECT(tx, ty, UI_ADDALARM_CLOSE_X, UI_ADDALARM_CLOSE_Y, UI_ADDALARM_CLOSE_W, UI_ADDALARM_CLOSE_H)) {
                            display_send(DISPLAY_SRC_SCREEN, "ALARM");
                        } else if (IN_RECT(tx, ty, UI_ADDALARM_CONFIRM_X, UI_ADDALARM_CONFIRM_Y, UI_ADDALARM_CONFIRM_W, UI_ADDALARM_CONFIRM_H)) {
                            display_send(DISPLAY_SRC_ALARM_SAVE, "SAVE");
                        } else if (IN_RECT(tx, ty, UI_ADDALARM_HOUR_UP_X, UI_ADDALARM_HOUR_UP_Y, UI_ADDALARM_HOUR_UP_W, UI_ADDALARM_HOUR_UP_H)) {
                            display_send(DISPLAY_SRC_ALARM_EDIT, "HOUR_UP");
                        } else if (IN_RECT(tx, ty, UI_ADDALARM_HOUR_DOWN_X, UI_ADDALARM_HOUR_DOWN_Y, UI_ADDALARM_HOUR_DOWN_W, UI_ADDALARM_HOUR_DOWN_H)) {
                            display_send(DISPLAY_SRC_ALARM_EDIT, "HOUR_DOWN");
                        } else if (IN_RECT(tx, ty, UI_ADDALARM_MIN_UP_X, UI_ADDALARM_MIN_UP_Y, UI_ADDALARM_MIN_UP_W, UI_ADDALARM_MIN_UP_H)) {
                            display_send(DISPLAY_SRC_ALARM_EDIT, "MIN_UP");
                        } else if (IN_RECT(tx, ty, UI_ADDALARM_MIN_DOWN_X, UI_ADDALARM_MIN_DOWN_Y, UI_ADDALARM_MIN_DOWN_W, UI_ADDALARM_MIN_DOWN_H)) {
                            display_send(DISPLAY_SRC_ALARM_EDIT, "MIN_DOWN");
                        } else if (IN_RECT(tx, ty, UI_ADDALARM_AMPM_X, UI_ADDALARM_AMPM_Y, UI_ADDALARM_AMPM_W, UI_ADDALARM_AMPM_H)) {
                            display_send(DISPLAY_SRC_ALARM_EDIT, "AMPM_TOGGLE");
                        } else {
                            /* kiểm tra 7 vòng tròn chọn thứ — hình tròn nên dùng khoảng cách, không dùng IN_RECT */
                            for (int i = 0; i < 7; i++) {
                                int32_t cx = UI_ADDALARM_DAY0_CX + i * UI_ADDALARM_DAY_STEP;
                                int32_t cy = UI_ADDALARM_DAY_CY;
                                int32_t dx = tx - cx, dy = ty - cy;
                                if (dx * dx + dy * dy <= UI_ADDALARM_DAY_R * UI_ADDALARM_DAY_R) {
                                    char cmd[16];
                                    snprintf(cmd, sizeof(cmd), "DAY_%d", i);
                                    display_send(DISPLAY_SRC_ALARM_EDIT, cmd);
                                    break;
                                }
                            }
                        }
                        break;

                    default:
                        break;
                    }
                    last_trigger = now;
                }
            }

        } else if (pressed && was_pressed && press_alarm_idx >= 0 && !long_fired) {
            /* ----- đang giữ tay trên 1 card alarm: đủ lâu chưa? ----- */
            if ((now - press_start) > pdMS_TO_TICKS(TOUCH_LONG_PRESS_MS)) {
                long_fired = true;
                char cmd[20];
                snprintf(cmd, sizeof(cmd), "DELETE_%d", press_alarm_idx);
                display_send(DISPLAY_SRC_ALARM_EDIT, cmd);
                last_trigger = now;
            }

        } else if (!pressed && was_pressed) {
            /* ----- vừa nhả tay ----- */
            if (press_alarm_idx >= 0) {
                if (!long_fired) {
                    /* nhả trước khi đủ long-press -> coi là chạm ngắn -> mở màn sửa */
                    char cmd[20];
                    snprintf(cmd, sizeof(cmd), "EDIT_%d", press_alarm_idx);
                    display_send(DISPLAY_SRC_ALARM_EDIT, cmd);
                }
                last_trigger = now;   /* debounce cho lần chạm tiếp theo */
            }
            press_alarm_idx = -1;
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