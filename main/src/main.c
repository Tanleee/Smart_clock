#include "nvs_flash.h"
#include "esp_err.h"

#include "display.h"
#include "wifi_sta.h"
#include "time_sync.h"
#include "alarm_data.h"

#include "alarm_task.h"
#include "buzzer.h"

void app_main(void)
{
    /* Khởi tạo NVS (WiFi cần) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

	ESP_ERROR_CHECK(alarm_data_init());
		
    /* Display phải được khởi tạo đầu tiên để các module khác gửi được dữ liệu */
    ESP_ERROR_CHECK(display_init());
    display_send(DISPLAY_SRC_SYSTEM, "Smart clock starting...");
	
	ESP_ERROR_CHECK(buzzer_init());

    wifi_sta_init();
    ESP_ERROR_CHECK(time_sync_start());
	
	ESP_ERROR_CHECK(alarm_task_start());
}
