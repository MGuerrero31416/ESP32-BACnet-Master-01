#include "app_system.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "../../main/User_Settings.h"

static const char *TAG = "app_system";

int override_nvs_on_flash = 0; /* Exported for AV/BV modules */

void app_system_init(void)
{
    esp_err_t ret = nvs_flash_init();

    /* If OVERRIDE_NVS_ON_FLASH is set, erase NVS to reset to code defaults.
     * Guard against wiping provisioned Wi-Fi when no compile-time defaults exist.
     */
    override_nvs_on_flash = USER_OVERRIDE_NVS_ON_FLASH;
    if (override_nvs_on_flash && USER_WIFI_SSID[0] == '\0') {
        ESP_LOGW(TAG, "OVERRIDE_NVS_ON_FLASH ignored: USER_WIFI_SSID is empty and erase would remove provisioned Wi-Fi credentials");
        override_nvs_on_flash = 0;
    }

    if (override_nvs_on_flash) {
        ESP_LOGI(TAG, "Override flag set - erasing NVS to reset to defaults");
        nvs_flash_erase();
        ret = nvs_flash_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reinitialize NVS after erase: %d", ret);
        } else {
            ESP_LOGI(TAG, "NVS reinitialized successfully");
        }
    } else if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "NVS needs initialization");
        nvs_flash_erase();
        nvs_flash_init();
    } else if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS initialized from existing data");
    }
}
