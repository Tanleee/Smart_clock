#pragma once

/* Tọa độ các thành phần trên dashboard 320x240 — trích từ bảng đo đạc. */

/* 1. Giờ "14:35" */
#define UI_CLOCK_X          15
#define UI_CLOCK_Y          50
#define UI_CLOCK_W          150   /* tăng từ 124 */
#define UI_CLOCK_H          50

/* 2. Ngày "Sat, Apr 26, 2025" */
#define UI_DATE_X           45
#define UI_DATE_Y           115
#define UI_DATE_W           130   /* tăng từ 83 */
#define UI_DATE_H           10

/* 3. Thứ "Saturday" */
#define UI_WEEKDAY_X        64
#define UI_WEEKDAY_Y        138
#define UI_WEEKDAY_W        70    /* tăng từ 36 */
#define UI_WEEKDAY_H        8

/* 6. Icon Wi-Fi */
#define UI_WIFI_ICON_X      264
#define UI_WIFI_ICON_Y      18
#define UI_WIFI_ICON_W      17
#define UI_WIFI_ICON_H      12

/* 7. Icon cường độ tín hiệu */
#define UI_SIGNAL_ICON_X    286
#define UI_SIGNAL_ICON_Y    18
#define UI_SIGNAL_ICON_W    15
#define UI_SIGNAL_ICON_H    12

/* 8. Icon thời tiết (mây + nắng) */
#define UI_WEATHER_ICON_X   183
#define UI_WEATHER_ICON_Y   59
#define UI_WEATHER_ICON_W   33
#define UI_WEATHER_ICON_H   28

/* 9. Chữ "Outdoor" */
#define UI_OUTDOOR_LABEL_X  228
#define UI_OUTDOOR_LABEL_Y  56
#define UI_OUTDOOR_LABEL_W  32
#define UI_OUTDOOR_LABEL_H  8

/* 10. Nhiệt độ ngoài trời "24°C" */
#define UI_TEMP_OUT_X       228
#define UI_TEMP_OUT_Y       66
#define UI_TEMP_OUT_W       40
#define UI_TEMP_OUT_H       18

/* 11. Chữ "Partly cloudy" */
#define UI_WEATHER_TEXT_X   228
#define UI_WEATHER_TEXT_Y   88
#define UI_WEATHER_TEXT_W   47
#define UI_WEATHER_TEXT_H   8

/* 12. Icon nhiệt kế lớn */
#define UI_THERMO_BIG_X     184
#define UI_THERMO_BIG_Y     129
#define UI_THERMO_BIG_W     11
#define UI_THERMO_BIG_H     25

/* 13. Icon giọt nước lớn */
#define UI_DROP_BIG_X       199
#define UI_DROP_BIG_Y       140
#define UI_DROP_BIG_W       10
#define UI_DROP_BIG_H       14

/* 15. Icon nhiệt kế nhỏ */
#define UI_THERMO_SM_X      229
#define UI_THERMO_SM_Y      130
#define UI_THERMO_SM_W      8
#define UI_THERMO_SM_H      14

/* 16. Nhiệt độ trong nhà "26.8°C" */
#define UI_TEMP_IN_X        243
#define UI_TEMP_IN_Y        132
#define UI_TEMP_IN_W        37
#define UI_TEMP_IN_H        12

/* 17. Icon giọt nước nhỏ */
#define UI_DROP_SM_X        229
#define UI_DROP_SM_Y        149
#define UI_DROP_SM_W        8
#define UI_DROP_SM_H        10

/* 18. Độ ẩm "58%" */
#define UI_HUMI_X           243
#define UI_HUMI_Y           148
#define UI_HUMI_W           23
#define UI_HUMI_H           12

/* 21. Nút menu tròn "≡" — vùng chạm để mở menu.h */
#define UI_MENU_BTN_X        274
#define UI_MENU_BTN_Y        187
#define UI_MENU_BTN_W        32
#define UI_MENU_BTN_H        32

// Tọa độ các thành phần trên menu

/* Nút back — vị trí cố định ở mọi menu/sub-screen */
#define UI_BACK_BTN_X        14
#define UI_BACK_BTN_Y        14
#define UI_BACK_BTN_W        28
#define UI_BACK_BTN_H        28

/* Khung 1: Alarm */
#define UI_MENU_ALARM_X      45
#define UI_MENU_ALARM_Y      46
#define UI_MENU_ALARM_W      110
#define UI_MENU_ALARM_H      72

/* Khung 2: Stopwatch */
#define UI_MENU_STOPWATCH_X  165
#define UI_MENU_STOPWATCH_Y  46
#define UI_MENU_STOPWATCH_W  110
#define UI_MENU_STOPWATCH_H  72

/* Khung 3: Pomodoro */
#define UI_MENU_POMODORO_X   45
#define UI_MENU_POMODORO_Y   128
#define UI_MENU_POMODORO_W   110
#define UI_MENU_POMODORO_H   72

/* Khung 4: Set Time */
#define UI_MENU_SETTIME_X    165
#define UI_MENU_SETTIME_Y    128
#define UI_MENU_SETTIME_W    110
#define UI_MENU_SETTIME_H    72

/* Nút "+" trên màn Alarm list */
#define UI_ALARM_ADD_BTN_X     276
#define UI_ALARM_ADD_BTN_Y     188
#define UI_ALARM_ADD_BTN_W     32
#define UI_ALARM_ADD_BTN_H     32

/* Danh sách báo thức trên màn Alarm — mỗi báo thức 1 khung round-rect.
 * Vùng trống còn lại: từ dưới back-btn (y=14+28=42) đến trên nút "+" (y=188) → cao ~146px. */
#define UI_ALARMLIST_X          14
#define UI_ALARMLIST_Y0         48     /* y của card đầu tiên */
#define UI_ALARMLIST_W          292
#define UI_ALARMLIST_H          40     /* chiều cao mỗi card */
#define UI_ALARMLIST_GAP        8      /* khoảng cách giữa 2 card */
#define UI_ALARMLIST_VISIBLE    3      /* số card hiển thị vừa, không cuộn — xem ghi chú trong display.cpp */

/* Màn New Alarm */
#define UI_ADDALARM_CLOSE_X        14
#define UI_ADDALARM_CLOSE_Y        13
#define UI_ADDALARM_CLOSE_W        28
#define UI_ADDALARM_CLOSE_H        28

#define UI_ADDALARM_CONFIRM_X      279
#define UI_ADDALARM_CONFIRM_Y      13
#define UI_ADDALARM_CONFIRM_W      28
#define UI_ADDALARM_CONFIRM_H      28

/* Vùng CHẠM của nút tăng/giảm giờ-phút, KHÔNG phải kích thước icon vẽ (icon vẫn
 * nằm nguyên trong ảnh nền add_alarm_bg). Mở rộng ra 2 bên + cao hơn nhiều so với
 * icon mũi tên gốc (12x9) để ngón tay dễ bấm trúng, vẫn canh giữa theo đúng tâm
 * icon cũ nên không cần sửa gì bên vẽ. Có thể cần chỉnh vài px sau khi test tay. */
#define UI_ADDALARM_HOUR_UP_X      97
#define UI_ADDALARM_HOUR_UP_Y      46
#define UI_ADDALARM_HOUR_UP_W      40
#define UI_ADDALARM_HOUR_UP_H      29

#define UI_ADDALARM_HOUR_DOWN_X    97
#define UI_ADDALARM_HOUR_DOWN_Y    137
#define UI_ADDALARM_HOUR_DOWN_W    40
#define UI_ADDALARM_HOUR_DOWN_H    29

/* Vùng vẽ số giờ — ước lượng giữa 2 mũi tên, có thể cần chỉnh vài px sau khi test */
#define UI_ADDALARM_HOUR_DIGIT_X   86
#define UI_ADDALARM_HOUR_DIGIT_Y   80
#define UI_ADDALARM_HOUR_DIGIT_W   61
#define UI_ADDALARM_HOUR_DIGIT_H   50

#define UI_ADDALARM_MIN_UP_X       184
#define UI_ADDALARM_MIN_UP_Y       46
#define UI_ADDALARM_MIN_UP_W       40
#define UI_ADDALARM_MIN_UP_H       29

#define UI_ADDALARM_MIN_DOWN_X     184
#define UI_ADDALARM_MIN_DOWN_Y     137
#define UI_ADDALARM_MIN_DOWN_W     40
#define UI_ADDALARM_MIN_DOWN_H     29

#define UI_ADDALARM_MIN_DIGIT_X    173
#define UI_ADDALARM_MIN_DIGIT_Y    80
#define UI_ADDALARM_MIN_DIGIT_W    61
#define UI_ADDALARM_MIN_DIGIT_H    50

#define UI_ADDALARM_AMPM_X         255
#define UI_ADDALARM_AMPM_Y         81
#define UI_ADDALARM_AMPM_W         23
#define UI_ADDALARM_AMPM_H         50

/* 7 vòng tròn chọn thứ — tâm & bán kính, cách đều 32px */
#define UI_ADDALARM_DAY0_CX        63
#define UI_ADDALARM_DAY_CY         195
#define UI_ADDALARM_DAY_R          12
#define UI_ADDALARM_DAY_STEP       32

/* Khung thông báo báo thức đang reo — hiện giữa màn 320x240, đè lên mọi màn hình khác */
#define UI_RING_CARD_X      40
#define UI_RING_CARD_Y      68
#define UI_RING_CARD_W      240
#define UI_RING_CARD_H      120

#define UI_RING_BTN_X        110
#define UI_RING_BTN_Y        150
#define UI_RING_BTN_W        100
#define UI_RING_BTN_H         28