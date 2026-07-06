#include "bacnet_mstp.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "User_Settings.h"
#include "mstp_rs485.h"
#include "bacnet_app.h"

#include "bacnet/bacapp.h"
#include "bacnet/basic/service/h_apdu.h"
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/service/s_iam.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"

static const char *TAG = "bacnet";

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

static volatile uint32_t mstp_pdu_count = 0;
static volatile uint32_t mstp_apdu_count = 0;
static volatile uint32_t mstp_rp_total = 0;
static volatile uint32_t mstp_wp_total = 0;
static float mstp_rp_last_value = 0.0f;
static uint32_t mstp_last_seen_pdu = 0;
static uint8_t mstp_alive_ticks = 0;

static uint8_t mstp_rx_buffer[512];
static uint8_t mstp_tx_buffer[512];
static struct mstp_port_struct_t mstp_port;
static struct dlmstp_user_data_t mstp_user;
static struct dlmstp_rs485_driver mstp_rs485_driver = {
    .init = MSTP_RS485_Init,
    .send = MSTP_RS485_Send,
    .read = MSTP_RS485_Read,
    .transmitting = MSTP_RS485_Transmitting,
    .baud_rate = MSTP_RS485_Baud_Rate,
    .baud_rate_set = MSTP_RS485_Baud_Rate_Set,
    .silence_milliseconds = MSTP_RS485_Silence_Milliseconds,
    .silence_reset = MSTP_RS485_Silence_Reset
};

static void bacnet_mstp_receive_task(void *pvParameters)
{
    (void)pvParameters;
    BACNET_ADDRESS src = {0};
    static uint8_t rx_buffer[600];
    uint16_t pdu_len = 0;

    ESP_LOGI(TAG, "BACnet MS/TP receive task started");

    while (1) {
        memset(&src, 0, sizeof(src));
        pdu_len = dlmstp_receive(&src, rx_buffer, sizeof(rx_buffer), 0);
        if (pdu_len > 0) {
            mstp_pdu_count++;
            BACNET_ADDRESS dest = {0};
            BACNET_NPDU_DATA npdu_data = {0};
            int apdu_offset = bacnet_npdu_decode(
                rx_buffer, pdu_len, &dest, &src, &npdu_data);
            if (apdu_offset > 0 && apdu_offset < (int)pdu_len) {
                mstp_apdu_count++;
                if ((apdu_offset + 4) <= (int)pdu_len) {
                    uint8_t pdu_type = rx_buffer[apdu_offset] & 0xF0;
                    uint8_t service_choice = rx_buffer[apdu_offset + 3];
                    if (pdu_type == PDU_TYPE_CONFIRMED_SERVICE_REQUEST &&
                        service_choice == SERVICE_CONFIRMED_READ_PROPERTY) {
                        mstp_rp_total++;
                        mstp_rp_last_value = Analog_Value_Present_Value(1);
                    } else if (pdu_type == PDU_TYPE_CONFIRMED_SERVICE_REQUEST &&
                        service_choice == SERVICE_CONFIRMED_WRITE_PROPERTY) {
                        mstp_wp_total++;
                    }
                }
                bacnet_app_log_whois_iam(&rx_buffer[apdu_offset], pdu_len - apdu_offset, "mstp");
                bacnet_app_datalink_lock(bacnet_app_datalink_mstp());
                apdu_handler(&src, &rx_buffer[apdu_offset], pdu_len - apdu_offset);
                bacnet_app_datalink_unlock();
            } else {
                ESP_LOGW(TAG, "MS/TP RX frame decode failed: len=%u apdu_offset=%d src.len=%u src.mac=%u",
                    (unsigned)pdu_len, apdu_offset, (unsigned)src.len,
                    (unsigned)(src.len ? src.mac[0] : 0));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

bool bacnet_mstp_init(void)
{
    MSTP_RS485_Init();

    memset(&mstp_port, 0, sizeof(mstp_port));
    memset(&mstp_user, 0, sizeof(mstp_user));

    mstp_user.RS485_Driver = &mstp_rs485_driver;
    mstp_port.UserData = &mstp_user;
    mstp_port.InputBuffer = mstp_rx_buffer;
    mstp_port.InputBufferSize = sizeof(mstp_rx_buffer);
    mstp_port.OutputBuffer = mstp_tx_buffer;
    mstp_port.OutputBufferSize = sizeof(mstp_tx_buffer);

    dlmstp_set_interface((const char *)&mstp_port);
    dlmstp_set_mac_address(USER_MSTP_MAC_ADDRESS);
    dlmstp_set_max_info_frames(USER_MSTP_MAX_INFO_FRAMES);
    dlmstp_set_max_master(USER_MSTP_MAX_MASTER);
    dlmstp_set_baud_rate(USER_MSTP_BAUD_RATE);
    dlmstp_check_auto_baud_set(USER_MSTP_AUTO_BAUD);
    dlmstp_slave_mode_enabled_set(false);

    ESP_LOGI(
        TAG,
        "MS/TP config: mac=%u max_master=%u max_info=%u baud=%lu auto_baud=%s",
        (unsigned)USER_MSTP_MAC_ADDRESS,
        (unsigned)USER_MSTP_MAX_MASTER,
        (unsigned)USER_MSTP_MAX_INFO_FRAMES,
        (unsigned long)USER_MSTP_BAUD_RATE,
        USER_MSTP_AUTO_BAUD ? "on" : "off");

    return dlmstp_init((char *)&mstp_port);
}

void bacnet_mstp_start(void)
{
    log_heap_state("before create bacnet_mstp_rx task");
    if (xTaskCreate(bacnet_mstp_receive_task, "bacnet_mstp_rx", 12288, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create bacnet_mstp_rx task");
    }
    dlmstp_reset_statistics();
}

void bacnet_mstp_send_i_am(void)
{
    bacnet_app_datalink_lock(bacnet_app_datalink_mstp());
    Send_I_Am(Handler_Transmit_Buffer);
    bacnet_app_datalink_unlock();
}

void bacnet_mstp_tick_1s(void)
{
    if (mstp_pdu_count != mstp_last_seen_pdu) {
        mstp_last_seen_pdu = mstp_pdu_count;
        mstp_alive_ticks = 6;
    } else if (mstp_alive_ticks > 0) {
        mstp_alive_ticks--;
    }
}

void bacnet_mstp_run_30s_diagnostics(void)
{
    uint32_t rx_bytes = MSTP_RS485_Rx_Bytes_Get_Reset();
    uint32_t preamble_55 = 0;
    uint32_t preamble_55ff = 0;
    struct dlmstp_statistics mstp_stats = {0};
    MSTP_RS485_Preamble_Counts_Get_Reset(&preamble_55, &preamble_55ff);
    dlmstp_fill_statistics(&mstp_stats);
#if !MSTP_DEBUG_ENABLE
    (void)rx_bytes;
    (void)preamble_55;
    (void)preamble_55ff;
#endif
    if (mstp_stats.bad_crc_counter > 0 ||
        mstp_stats.receive_invalid_frame_counter > 0 ||
        mstp_stats.lost_token_counter > 0) {
        ESP_LOGW(
            TAG,
            "MS/TP errors(30s): bad_crc=%lu invalid=%lu lost_token=%lu",
            (unsigned long)mstp_stats.bad_crc_counter,
            (unsigned long)mstp_stats.receive_invalid_frame_counter,
            (unsigned long)mstp_stats.lost_token_counter);
    }
#if MSTP_DEBUG_ENABLE
    else {
        ESP_LOGD(
            TAG,
            "MS/TP 30s stats: rx_bytes=%lu preamble55=%lu preamble55ff=%lu pdu=%lu apdu=%lu rp=%lu wp=%lu valid=%lu invalid=%lu not_for_us=%lu bad_crc=%lu tx_frames=%lu rx_pdu=%lu poll=%lu lost_token=%lu sole_master=%d",
            (unsigned long)rx_bytes,
            (unsigned long)preamble_55,
            (unsigned long)preamble_55ff,
            (unsigned long)mstp_pdu_count,
            (unsigned long)mstp_apdu_count,
            (unsigned long)mstp_rp_total,
            (unsigned long)mstp_wp_total,
            (unsigned long)mstp_stats.receive_valid_frame_counter,
            (unsigned long)mstp_stats.receive_invalid_frame_counter,
            (unsigned long)mstp_stats.receive_valid_frame_not_for_us_counter,
            (unsigned long)mstp_stats.bad_crc_counter,
            (unsigned long)mstp_stats.transmit_frame_counter,
            (unsigned long)mstp_stats.receive_pdu_counter,
            (unsigned long)mstp_stats.poll_for_master_counter,
            (unsigned long)mstp_stats.lost_token_counter,
            dlmstp_sole_master() ? 1 : 0);
    }
#endif
    mstp_pdu_count = 0;
    mstp_apdu_count = 0;
    mstp_rp_total = 0;
    mstp_wp_total = 0;
    dlmstp_reset_statistics();
}

bool bacnet_mstp_is_link_alive(void)
{
    return mstp_alive_ticks > 0;
}

void *bacnet_mstp_port_handle(void)
{
    return &mstp_port;
}
