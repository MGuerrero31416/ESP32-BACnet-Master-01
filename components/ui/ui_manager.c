#include "ui_manager.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "../../main/User_Settings.h"
#include "../../main/display.h"
#include "bacnet_ip.h"
#include "bacnet_mstp.h"
#include "bacnet/bacapp.h"
#include "bacnet/basic/object/av.h"

static const char *TAG = "ui_manager";

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
            char ip_text[20];
            bacnet_ip_get_ip_string(ip_text, sizeof(ip_text));
            display_update_values(av1, av2, av3, av4);
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
    ESP_LOGI(TAG, "Initializing display");
    display_init();

    if (xTaskCreate(ui_manager_task, "ui_manager", 4096, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ui_manager task");
    }
}
