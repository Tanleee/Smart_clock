#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "alarm_data.h"

static const char *TAG = "alarm_data";
static const char *NVS_NAMESPACE = "alarm_ns";
static const char *NVS_KEY = "alarms";

static alarm_t s_alarms[ALARM_MAX_COUNT];

static esp_err_t save_to_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(handle, NVS_KEY, s_alarms, sizeof(s_alarms));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t alarm_data_init(void)
{
    memset(s_alarms, 0, sizeof(s_alarms));
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return err;
    }
    size_t required_size = sizeof(s_alarms);
    err = nvs_get_blob(handle, NVS_KEY, s_alarms, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved alarms yet");
        err = ESP_OK;
    }
    nvs_close(handle);
    return err;
}

int alarm_data_count(void)
{
    int count = 0;
    for (int i = 0; i < ALARM_MAX_COUNT; i++) if (s_alarms[i].valid) count++;
    return count;
}

alarm_t *alarm_data_get(int index)
{
    int count = 0;
    for (int i = 0; i < ALARM_MAX_COUNT; i++) {
        if (s_alarms[i].valid) {
            if (count == index) return &s_alarms[i];
            count++;
        }
    }
    return NULL;
}

esp_err_t alarm_data_add(const alarm_t *a)
{
    for (int i = 0; i < ALARM_MAX_COUNT; i++) {
        if (!s_alarms[i].valid) {
            s_alarms[i] = *a;
            s_alarms[i].valid = true;
            return save_to_nvs();
        }
    }
    return ESP_ERR_NO_MEM;
}

esp_err_t alarm_data_update(int index, const alarm_t *a)
{
    /* ghi đè đúng ô vật lý đang ứng với "báo thức thứ index" trong danh sách
     * hợp lệ hiện tại (giữ nguyên vị trí lưu trữ, chỉ đổi nội dung) */
    alarm_t *slot = alarm_data_get(index);
    if (!slot) return ESP_ERR_NOT_FOUND;
    *slot = *a;
    slot->valid = true;
    return save_to_nvs();
}

esp_err_t alarm_data_remove(int index)
{
    alarm_t *a = alarm_data_get(index);
    if (!a) return ESP_ERR_NOT_FOUND;
    memset(a, 0, sizeof(*a));
    return save_to_nvs();
}

esp_err_t alarm_data_set_enabled(int index, bool enabled)
{
    alarm_t *a = alarm_data_get(index);
    if (!a) return ESP_ERR_NOT_FOUND;
    a->enabled = enabled;
    return save_to_nvs();
}