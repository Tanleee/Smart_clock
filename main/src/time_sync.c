#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"

#include "app_config.h"
#include "display.h"
#include "wifi_sta.h"
#include "time_sync.h"

static const char *TAG = "time_sync";

/* Danh sách server NTP, thử lần lượt */
static const char *s_ntp_servers[] = {
    "time.google.com",
    "time.cloudflare.com",
    "vn.pool.ntp.org",
    "pool.ntp.org",
    "216.239.35.0",     /* IP của time.google.com (không cần DNS) */
    "162.159.200.1",    /* IP của time.cloudflare.com (không cần DNS) */
};
#define SNTP_SERVER_COUNT   (sizeof(s_ntp_servers) / sizeof(s_ntp_servers[0]))
#define SNTP_TRY_PER_SERVER 3

static void time_task(void *pvParameters)
{
    /* 1. Chờ WiFi kết nối */
    if (!wifi_sta_wait_connected(portMAX_DELAY)) {
        ESP_LOGE(TAG, "WiFi connection failed, stop time task");
        vTaskDelete(NULL);
        return;
    }

    /* 2. Thêm DNS dự phòng 8.8.8.8 */
    esp_netif_dns_info_t dns = { 0 };
    dns.ip.type = ESP_IPADDR_TYPE_V4;
    dns.ip.u_addr.ip4.addr = ESP_IP4TOADDR(8, 8, 8, 8);
    esp_netif_set_dns_info(wifi_sta_get_netif(), ESP_NETIF_DNS_BACKUP, &dns);

    /* 3. Thử lần lượt từng server NTP */
    bool synced = false;
    for (int s = 0; s < (int)SNTP_SERVER_COUNT && !synced; s++) {
        display_send(DISPLAY_SRC_SNTP, "Trying %s", s_ntp_servers[s]);
        esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(s_ntp_servers[s]);
        ESP_ERROR_CHECK(esp_netif_sntp_init(&sntp_cfg));

        for (int r = 0; r < SNTP_TRY_PER_SERVER; r++) {
            if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(3000)) == ESP_OK) {
                synced = true;
                display_send(DISPLAY_SRC_SNTP, "Synced via %s", s_ntp_servers[s]);
                break;
            }
            ESP_LOGW(TAG, "No response from %s (%d/%d)", s_ntp_servers[s], r + 1, SNTP_TRY_PER_SERVER);
        }

        if (!synced) {
            esp_netif_sntp_deinit();
        }
    }

    if (!synced) {
        display_send(DISPLAY_SRC_SNTP, "All NTP servers failed");
    }

    /* 4. Đặt múi giờ */
    setenv("TZ", TIME_ZONE, 1);
    tzset();

    /* 5. Gửi giờ vào queue định kỳ */
    while (1) {
        time_t now;
        struct tm timeinfo;
        char buf[DISPLAY_TEXT_MAX_LEN];

        time(&now);
        localtime_r(&now, &timeinfo);

        if (timeinfo.tm_year < (2024 - 1900)) {
            display_send(DISPLAY_SRC_TIME, "Time not synchronized yet");
        } else {
            strftime(buf, sizeof(buf), "%a %d/%m/%Y %H:%M:%S", &timeinfo);
            display_send(DISPLAY_SRC_TIME, "%s", buf);
        }

        vTaskDelay(pdMS_TO_TICKS(PRINT_PERIOD_MS));
    }
}

esp_err_t time_sync_start(void)
{
    return (xTaskCreate(time_task, "time_task", TIME_TASK_STACK, NULL, TIME_TASK_PRIO, NULL) == pdPASS)
           ? ESP_OK : ESP_FAIL;
}
