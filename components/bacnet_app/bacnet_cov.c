#include "bacnet_cov.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "User_Settings.h"
#include "bacnet_app.h"

#include "bacnet/basic/service/h_cov.h"

static const char *TAG = "bacnet";
static TaskHandle_t bacnet_cov_task_handle = NULL;

static void log_heap_state(const char *context)
{
    ESP_LOGI(
        TAG,
        "[HEAP] %s free=%u min=%u internal=%u",
        context,
        (unsigned)esp_get_free_heap_size(),
        (unsigned)esp_get_minimum_free_heap_size(),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}

static void bacnet_cov_task(void *pvParameters)
{
    (void)pvParameters;
    while (1) {
        char *active_datalink = bacnet_app_datalink_default();
        if (!active_datalink) {
            if (USER_ENABLE_BACNET_IP) {
                active_datalink = bacnet_app_datalink_bip();
            } else if (USER_ENABLE_BACNET_MSTP) {
                active_datalink = bacnet_app_datalink_mstp();
            }
        }

        if (active_datalink) {
            bacnet_app_datalink_lock(active_datalink);
            handler_cov_timer_seconds(1);
            handler_cov_task();
            bacnet_app_datalink_unlock();
        } else {
            handler_cov_timer_seconds(1);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void bacnet_cov_start(void)
{
    log_heap_state("before create bacnet_cov task");
    if (xTaskCreate(bacnet_cov_task, "bacnet_cov", 20480, NULL, 4, &bacnet_cov_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create bacnet_cov task");
    }
}
