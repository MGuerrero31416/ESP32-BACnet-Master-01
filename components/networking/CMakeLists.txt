idf_component_register(
    SRCS "wifi_helper.c"
    INCLUDE_DIRS "."
    REQUIRES product_config system freertos esp_event esp_wifi esp_netif nvs_flash lwip
)
