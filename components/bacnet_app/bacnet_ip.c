#include "bacnet_ip.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "../../main/User_Settings.h"
#include "bacnet_app.h"

#include "bacnet/bacapp.h"
#include "bacnet/bacaddr.h"
#include "bacnet/basic/service/h_apdu.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/service/s_iam.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/bip.h"

static const char *TAG = "bacnet";

static void bacnet_receive_task(void *pvParameters)
{
    (void)pvParameters;
    BACNET_ADDRESS src = {0};
    static uint8_t rx_buffer[600];
    uint16_t pdu_len = 0;

    ESP_LOGI(TAG, "BACnet receive task started");

    while (1) {
        memset(&src, 0, sizeof(src));
        pdu_len = bip_receive(&src, rx_buffer, sizeof(rx_buffer), 100);
        if (pdu_len > 0) {
            BACNET_ADDRESS orig_src = src;
            BACNET_ADDRESS dest = {0};
            BACNET_NPDU_DATA npdu_data = {0};
            int apdu_offset = bacnet_npdu_decode(
                rx_buffer, pdu_len, &dest, &src, &npdu_data);
            if (src.len == 0) {
                src = orig_src;
            }
            if (apdu_offset > 0 && apdu_offset < (int)pdu_len) {
                bacnet_app_log_whois_iam(&rx_buffer[apdu_offset], pdu_len - apdu_offset, "bip");
                bacnet_app_datalink_lock(bacnet_app_datalink_bip());
                apdu_handler(&src, &rx_buffer[apdu_offset], pdu_len - apdu_offset);
                bacnet_app_datalink_unlock();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void bacnet_ip_register_with_bbmd(void)
{
    BACNET_IP_ADDRESS bbmd_addr = { { USER_BBMD_IP_OCTET_1, USER_BBMD_IP_OCTET_2,
                                     USER_BBMD_IP_OCTET_3, USER_BBMD_IP_OCTET_4 },
                                    USER_BBMD_PORT };
    int result = bvlc_register_with_bbmd(&bbmd_addr, USER_BBMD_TTL_SECONDS);
    ESP_LOGI(TAG, "BBMD register result: %d", result);
}

void bacnet_ip_start(void)
{
    if (xTaskCreate(bacnet_receive_task, "bacnet_rx", 16384, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create bacnet_rx task");
    }
}

void bacnet_ip_send_i_am(void)
{
    bacnet_app_datalink_lock(bacnet_app_datalink_bip());
    Send_I_Am(Handler_Transmit_Buffer);
    bacnet_app_datalink_unlock();
}

bool bacnet_ip_is_connected(void)
{
    if (!USER_ENABLE_BACNET_IP) {
        return false;
    }

    wifi_ap_record_t ap_info = {0};
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}

void bacnet_ip_get_ip_string(char *out, size_t out_len)
{
    if (!out || out_len == 0) {
        return;
    }

    out[0] = '\0';
    if (!USER_ENABLE_BACNET_IP) {
        snprintf(out, out_len, "No IP");
        return;
    }

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        snprintf(out, out_len, "No IP");
        return;
    }

    esp_netif_ip_info_t ip_info = {0};
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
        snprintf(out, out_len, IPSTR, IP2STR(&ip_info.ip));
    } else {
        snprintf(out, out_len, "No IP");
    }
}
