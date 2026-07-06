idf_component_register(
    SRCS "bacnet_app.c" "bacnet_ip.c" "bacnet_mstp.c" "bacnet_cov.c"
         "../../main/wifi_helper.c"
         "../../main/mstp_rs485.c"
         "../../main/User_Settings.c"
    INCLUDE_DIRS "."
    REQUIRES bacnet-stack bacnet_objects freertos esp_event esp_wifi esp_netif nvs_flash driver esp_driver_i2c esp_driver_spi esp_driver_uart esp_lcd esp_timer lvgl__lvgl sen54
)
