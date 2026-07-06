#include "ui_manager.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "User_Settings.h"
#include "display.h"
#include "bacnet_ip.h"
#include "bacnet_mstp.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bv.h"

static const char *TAG = "ui_manager";

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

static void ui_manager_task(void *pvParameters)
{
    (void)pvParameters;

    uint32_t display_tick = 0;

    while (1) {
        display_set_link_status(
            bacnet_ip_is_connected(),
            USER_ENABLE_BACNET_MSTP && bacnet_mstp_is_link_alive());

        if (++display_tick % 2 == 0) {
            float av1 = Analog_Value_Present_Value(1);
            float av2 = Analog_Value_Present_Value(2);
            float av3 = Analog_Value_Present_Value(3);
            float av4 = Analog_Value_Present_Value(4);
            bool measurement_enabled =
                Binary_Value_Present_Value(2) == BINARY_ACTIVE;
            bool fan_failure = Binary_Input_Present_Value(1) == BINARY_ACTIVE;
            bool laser_error = Binary_Input_Present_Value(2) == BINARY_ACTIVE;
            bool voc_error = Binary_Input_Present_Value(3) == BINARY_ACTIVE;
            bool rht_error = Binary_Input_Present_Value(4) == BINARY_ACTIVE;
            char ip_text[20];

            bacnet_ip_get_ip_string(ip_text, sizeof(ip_text));
            display_update_values(av1, av2, av3, av4);
            display_update_sen54_controls(measurement_enabled);
            display_update_sen54_diagnostics(
                fan_failure,
                laser_error,
                voc_error,
                rht_error);
            display_update_footer(
                USER_BACNET_DEVICE_INSTANCE,
                USER_MSTP_MAC_ADDRESS,
                ip_text);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void ui_manager_start(void)
{
    log_heap_state("before display init");
    ESP_LOGI(TAG, "Initializing display");
    display_init();

    log_heap_state("before create ui_manager task");
    if (xTaskCreate(ui_manager_task, "ui_manager", 4096, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ui_manager task");
    }
}
