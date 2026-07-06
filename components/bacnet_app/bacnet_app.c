#include "bacnet_app.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "User_Settings.h"
#include "analog_input.h"
#include "analog_value.h"
#include "binary_input.h"
#include "binary_output.h"
#include "binary_value.h"
#include "wifi_helper.h"
#include "bacnet_cov.h"
#include "bacnet_ip.h"
#include "bacnet_mstp.h"

#include "bacnet/bacapp.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/service/h_cov.h"
#include "bacnet/basic/service/h_iam.h"
#include "bacnet/basic/service/h_rp.h"
#include "bacnet/basic/service/h_rpm.h"
#include "bacnet/basic/service/h_whois.h"
#include "bacnet/basic/service/h_wp.h"
#include "bacnet/basic/service/s_whois.h"
#include "bacnet/basic/service/s_iam.h"
#include "bacnet/basic/services.h"
#include "bacnet/bacenum.h"
#include "bacnet/iam.h"
#include "bacnet/whois.h"
#include "bacnet/datalink/datalink.h"

static const char *TAG = "bacnet";

static SemaphoreHandle_t bacnet_datalink_mutex = NULL;
static char datalink_bip[] = "bip";
static char datalink_mstp[] = "mstp";
static char *datalink_default = NULL;

static void bacnet_maintenance_task(void *pvParameters)
{
    (void)pvParameters;

    uint32_t iam_tick = 0;
    uint32_t mstp_rx_tick = 0;

    while (1) {
        if (USER_ENABLE_BACNET_IP) {
            bacnet_app_datalink_lock(datalink_bip);
            datalink_maintenance_timer(1);
            bacnet_app_datalink_unlock();
        }

        if (USER_ENABLE_BACNET_MSTP) {
            bacnet_mstp_tick_1s();

            if (++iam_tick % 60 == 0) {
                bacnet_mstp_send_i_am();
            }

            if (++mstp_rx_tick % 30 == 0) {
                bacnet_mstp_run_30s_diagnostics();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void bacnet_app_datalink_lock(char *name)
{
    if (bacnet_datalink_mutex) {
        xSemaphoreTake(bacnet_datalink_mutex, portMAX_DELAY);
    }
    datalink_set(name);
}

void bacnet_app_datalink_unlock(void)
{
    if (datalink_default) {
        datalink_set(datalink_default);
    }
    if (bacnet_datalink_mutex) {
        xSemaphoreGive(bacnet_datalink_mutex);
    }
}

char *bacnet_app_datalink_bip(void)
{
    return datalink_bip;
}

char *bacnet_app_datalink_mstp(void)
{
    return datalink_mstp;
}

char *bacnet_app_datalink_default(void)
{
    return datalink_default;
}

void bacnet_app_log_whois_iam(const uint8_t *apdu, int apdu_len, const char *link)
{
    if (!apdu || apdu_len < 2) {
        return;
    }

    uint8_t pdu_type = apdu[0] & 0xF0;
    if (pdu_type != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) {
        return;
    }

    uint8_t service_choice = apdu[1];
    if (service_choice == SERVICE_UNCONFIRMED_WHO_IS) {
        int32_t low_limit = -1;
        int32_t high_limit = -1;
        int len = whois_decode_service_request(
            &apdu[2], (unsigned)(apdu_len - 2), &low_limit, &high_limit);
        if (len >= 0) {
            bool in_range = true;
            if (low_limit >= 0 && high_limit >= 0) {
                uint32_t instance = USER_BACNET_DEVICE_INSTANCE;
                in_range = (instance >= (uint32_t)low_limit &&
                    instance <= (uint32_t)high_limit);
            }
            (void)in_range;
            (void)low_limit;
            (void)high_limit;
        }
    } else if (service_choice == SERVICE_UNCONFIRMED_I_AM) {
        uint32_t device_id = BACNET_MAX_INSTANCE;
        unsigned max_apdu = 0;
        int segmentation = SEGMENTATION_NONE;
        uint16_t vendor_id = 0;
        int len = iam_decode_service_request(
            &apdu[2], &device_id, &max_apdu, &segmentation, &vendor_id);
        if (len >= 0) {
            ESP_LOGI(
                TAG,
                "%s I-Am device=%lu vendor=%u max_apdu=%u",
                link,
                (unsigned long)device_id,
                (unsigned)vendor_id,
                max_apdu);
        } else {
            ESP_LOGW(TAG, "%s I-Am decode failed len=%d", link, apdu_len);
        }
    }
}

void bacnet_app_init(void)
{
    bacnet_datalink_mutex = xSemaphoreCreateMutex();
    if (!bacnet_datalink_mutex) {
        ESP_LOGE(TAG, "Failed to create BACnet datalink mutex");
    }

    if (USER_ENABLE_BACNET_IP) {
        esp_netif_init();
        esp_event_loop_create_default();
        wifi_init_sta();

        ESP_LOGI(TAG, "Initializing BACnet stack (B/IP)");
        datalink_set(datalink_bip);
        if (!datalink_init(NULL)) {
            ESP_LOGE(TAG, "Failed to initialize BACnet datalink");
        }

        bacnet_ip_register_with_bbmd();
    }

    if (USER_ENABLE_BACNET_MSTP) {
        ESP_LOGI(TAG, "Initializing BACnet MS/TP");
        if (!bacnet_mstp_init()) {
            ESP_LOGE(TAG, "Failed to initialize BACnet MS/TP datalink");
        } else {
            datalink_set(datalink_mstp);
            if (!datalink_init((char *)bacnet_mstp_port_handle())) {
                ESP_LOGE(TAG, "Failed to initialize BACnet MS/TP datalink interface");
            }
        }
    }

    if (USER_ENABLE_BACNET_IP) {
        datalink_default = datalink_bip;
    } else if (USER_ENABLE_BACNET_MSTP) {
        datalink_default = datalink_mstp;
    }
    if (datalink_default) {
        datalink_set(datalink_default);
    }

    Device_Init(NULL);
    User_Settings_InitDeviceIdentity();

    ESP_LOGI(TAG, "Registering BACnet service handlers");
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_add);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY, handler_cov_subscribe_property);

    handler_cov_init();

    bacnet_create_analog_values();
    bacnet_create_binary_values();
    bacnet_create_analog_inputs();
    bacnet_create_binary_inputs();
    bacnet_create_binary_outputs_with_gpio_sync();

    ESP_LOGI(TAG, "Broadcasting I-Am");
    if (USER_ENABLE_BACNET_IP) {
        bacnet_ip_send_i_am();
    }
    if (USER_ENABLE_BACNET_MSTP) {
        bacnet_mstp_send_i_am();
    }
}

void bacnet_app_start(void)
{
    if (USER_ENABLE_BACNET_IP) {
        bacnet_ip_start();
    }
    if (USER_ENABLE_BACNET_MSTP) {
        bacnet_mstp_start();
        ESP_LOGI(TAG, "BACnet MS/TP ready");
    }

    bacnet_cov_start();

    if (xTaskCreate(bacnet_maintenance_task, "bacnet_maint", 6144, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create bacnet_maint task");
    }
}
