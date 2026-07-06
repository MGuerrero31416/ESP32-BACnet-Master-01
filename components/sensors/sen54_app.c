#include "sen54_app.h"

#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "../../main/User_Settings.h"
#include "sen54.h"

#include "bacnet/bacapp.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bv.h"

static const char *TAG = "bacnet";

#define SEN54_AUTO_CLEANING_AV_INSTANCE 7U
#define SEN54_MEASUREMENT_ENABLE_BV_INSTANCE 2U
#define SEN54_FAN_CLEANING_BV_INSTANCE 3U
#define SEN54_CLEAR_STATUS_BV_INSTANCE 4U

#define SEN54_STATUS_FAN_ERROR_BIT  (1UL << 4)
#define SEN54_STATUS_LASER_ERROR_BIT (1UL << 5)
#define SEN54_STATUS_RHT_ERROR_BIT  (1UL << 6)
#define SEN54_STATUS_VOC_ERROR_BIT  (1UL << 7)

static bool sen54_auto_cleaning_interval_valid = false;
static uint32_t sen54_auto_cleaning_interval_seconds = 0;

static void sen54_set_command_bv_inactive(uint32_t instance)
{
    Binary_Value_Present_Value_Set(instance, BINARY_INACTIVE);
}

static void sen54_set_status_bi_if_changed(uint32_t instance,
                                           bool active,
                                           const char *label)
{
    BACNET_BINARY_PV current;
    BACNET_BINARY_PV next;

    current = Binary_Input_Present_Value(instance);
    next = active ? BINARY_ACTIVE : BINARY_INACTIVE;

    if (current != next) {
        Binary_Input_Present_Value_Set(instance, next);
        ESP_LOGI(TAG, "[SEN54] %s: %s", label, active ? "ON" : "OFF");
    }
}

static void sen54_update_status_objects(uint32_t status)
{
    sen54_set_status_bi_if_changed(1, (status & SEN54_STATUS_FAN_ERROR_BIT) != 0,
                                   "Fan Failure");
    sen54_set_status_bi_if_changed(2, (status & SEN54_STATUS_LASER_ERROR_BIT) != 0,
                                   "Laser Error");
    sen54_set_status_bi_if_changed(3, (status & SEN54_STATUS_VOC_ERROR_BIT) != 0,
                                   "VOC Sensor Error");
    sen54_set_status_bi_if_changed(4, (status & SEN54_STATUS_RHT_ERROR_BIT) != 0,
                                   "RHT Sensor Error");
}

static bool sen54_poll_and_publish_status(void)
{
    static bool status_valid = false;
    static uint32_t last_status = 0;
    uint32_t status = 0;

    if (!sen54_read_device_status(&status)) {
        ESP_LOGW(TAG, "[SEN54] Device status read failed");
        return false;
    }

    if (!status_valid || (status != last_status)) {
        ESP_LOGI(TAG, "[SEN54] Device Status = 0x%08lX", (unsigned long)status);
        sen54_update_status_objects(status);
        last_status = status;
        status_valid = true;
    }

    return true;
}

static bool sen54_publish_auto_cleaning_interval(uint32_t seconds)
{
    if (!Analog_Value_Present_Value_Set(
            SEN54_AUTO_CLEANING_AV_INSTANCE, (float)seconds, 16)) {
        return false;
    }

    sen54_auto_cleaning_interval_seconds = seconds;
    sen54_auto_cleaning_interval_valid = true;

    return true;
}

static void sen54_init_auto_cleaning_interval(void)
{
    uint32_t seconds = 0;

    if (sen54_read_auto_cleaning_interval(&seconds)) {
        if (sen54_publish_auto_cleaning_interval(seconds)) {
            ESP_LOGI(TAG, "[SEN54] Auto Cleaning Interval = %lu s",
                (unsigned long)seconds);
        } else {
            ESP_LOGE(TAG,
                "[SEN54] Auto Cleaning Interval read succeeded but AV%u update failed",
                (unsigned)SEN54_AUTO_CLEANING_AV_INSTANCE);
        }
    } else {
        ESP_LOGE(TAG, "[SEN54] Auto Cleaning Interval read failed");
    }
}

bool bacnet_av7_auto_cleaning_interval_write(
    float requested_value,
    float *applied_value,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    uint32_t requested_seconds = 0;
    uint32_t old_seconds = 0;

    if (error_class) {
        *error_class = ERROR_CLASS_PROPERTY;
    }
    if (error_code) {
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
    }

    if (!isfinite(requested_value) || requested_value <= 0.0f ||
        requested_value > 4294967295.0f) {
        return false;
    }

    requested_seconds = (uint32_t)requested_value;
    if ((float)requested_seconds != requested_value) {
        return false;
    }

    if (!sen54_write_auto_cleaning_interval(requested_seconds)) {
        if (error_class) {
            *error_class = ERROR_CLASS_DEVICE;
        }
        if (error_code) {
            *error_code = ERROR_CODE_OPERATIONAL_PROBLEM;
        }
        return false;
    }

    old_seconds = sen54_auto_cleaning_interval_valid
        ? sen54_auto_cleaning_interval_seconds
        : (uint32_t)Analog_Value_Present_Value(SEN54_AUTO_CLEANING_AV_INSTANCE);

    if (old_seconds != requested_seconds) {
        ESP_LOGI(TAG, "[SEN54] Auto Cleaning Interval changed:");
        ESP_LOGI(TAG, "Old: %lu s", (unsigned long)old_seconds);
        ESP_LOGI(TAG, "New: %lu s", (unsigned long)requested_seconds);
    }

    sen54_auto_cleaning_interval_seconds = requested_seconds;
    sen54_auto_cleaning_interval_valid = true;

    if (applied_value) {
        *applied_value = (float)requested_seconds;
    }

    return true;
}

static void sen54_task(void *pvParameters)
{
    (void)pvParameters;
    sen54_data_t sensor_data;
    uint8_t consecutive_failures = 0;
    TickType_t last_status_poll = 0;
    const TickType_t status_poll_interval_ticks = pdMS_TO_TICKS(5000);

    vTaskDelay(pdMS_TO_TICKS(3000));

    sen54_init();
    if (sen54_is_measurement_enabled()) {
        Binary_Value_Present_Value_Set(SEN54_MEASUREMENT_ENABLE_BV_INSTANCE, BINARY_ACTIVE);
    }
    sen54_set_command_bv_inactive(SEN54_FAN_CLEANING_BV_INSTANCE);
    sen54_set_command_bv_inactive(SEN54_CLEAR_STATUS_BV_INSTANCE);
    sen54_init_auto_cleaning_interval();

    sen54_poll_and_publish_status();
    last_status_poll = xTaskGetTickCount();

    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1) {
        if ((xTaskGetTickCount() - last_status_poll) >= status_poll_interval_ticks) {
            sen54_poll_and_publish_status();
            last_status_poll = xTaskGetTickCount();
        }

        if (Binary_Value_Present_Value(1) == BINARY_ACTIVE) {
            ESP_LOGI(TAG, "[SEN54] Full reset command requested (BV1 ACTIVE)");
            esp_err_t err = sen54_full_reset();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "[SEN54] Full reset command failed (%s)", esp_err_to_name(err));
            }
            sen54_set_command_bv_inactive(1);
            Binary_Value_Present_Value_Set(SEN54_MEASUREMENT_ENABLE_BV_INSTANCE,
                sen54_is_measurement_enabled() ? BINARY_ACTIVE : BINARY_INACTIVE);

            if (sen54_poll_and_publish_status()) {
                last_status_poll = xTaskGetTickCount();
            }
            continue;
        }

        {
            BACNET_BINARY_PV bv2 = Binary_Value_Present_Value(SEN54_MEASUREMENT_ENABLE_BV_INSTANCE);
            bool currently_enabled = sen54_is_measurement_enabled();

            if (bv2 == BINARY_ACTIVE && !currently_enabled) {
                if (!sen54_start_measurement()) {
                    Binary_Value_Present_Value_Set(SEN54_MEASUREMENT_ENABLE_BV_INSTANCE, BINARY_INACTIVE);
                }
            } else if (bv2 == BINARY_INACTIVE && currently_enabled) {
                if (!sen54_stop_measurement()) {
                    Binary_Value_Present_Value_Set(SEN54_MEASUREMENT_ENABLE_BV_INSTANCE, BINARY_ACTIVE);
                }
            }
        }

        if (Binary_Value_Present_Value(SEN54_FAN_CLEANING_BV_INSTANCE) == BINARY_ACTIVE) {
            sen54_start_fan_cleaning();
            sen54_set_command_bv_inactive(SEN54_FAN_CLEANING_BV_INSTANCE);
        }

        if (Binary_Value_Present_Value(SEN54_CLEAR_STATUS_BV_INSTANCE) == BINARY_ACTIVE) {
            if (sen54_clear_device_status()) {
                if (sen54_poll_and_publish_status()) {
                    last_status_poll = xTaskGetTickCount();
                } else {
                    ESP_LOGE(TAG, "[SEN54] Device status refresh after clear failed");
                }
            }

            sen54_set_command_bv_inactive(SEN54_CLEAR_STATUS_BV_INSTANCE);
        }

        if (sen54_is_measurement_enabled()) {
            if (sen54_read(&sensor_data)) {
                consecutive_failures = 0;
                Analog_Value_Present_Value_Set(1, sensor_data.temperature, 16);
                Analog_Value_Present_Value_Set(2, sensor_data.humidity,    16);
                Analog_Value_Present_Value_Set(3, sensor_data.pm2_5,       16);
                Analog_Value_Present_Value_Set(4, sensor_data.voc_index,   16);
                Analog_Value_Present_Value_Set(5, sensor_data.pm1_0,       16);
                Analog_Value_Present_Value_Set(6, sensor_data.pm4_0,       16);
            } else {
                consecutive_failures++;

                Analog_Value_Present_Value_Set(1, -1.0f, 16);
                Analog_Value_Present_Value_Set(2, -1.0f, 16);
                Analog_Value_Present_Value_Set(3, -1.0f, 16);
                Analog_Value_Present_Value_Set(4, -1.0f, 16);
                Analog_Value_Present_Value_Set(5, -1.0f, 16);
                Analog_Value_Present_Value_Set(6, -1.0f, 16);

                if (consecutive_failures >= 5) {
                    ESP_LOGW(TAG, "SEN54 read failed %u times, reinitializing sensor",
                             (unsigned)consecutive_failures);
                    sen54_init();
                    if (sen54_poll_and_publish_status()) {
                        last_status_poll = xTaskGetTickCount();
                    }
                    consecutive_failures = 0;
                    vTaskDelay(pdMS_TO_TICKS(1500));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void sen54_app_start(void)
{
    if (xTaskCreate(sen54_task, "sen54", 4096, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sen54 task");
    }
}
