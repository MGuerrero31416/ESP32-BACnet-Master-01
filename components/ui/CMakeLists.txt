idf_component_register(
    SRCS "ui_manager.c"
         "legacy_display.cpp"
         "legacy_touch_bsp.c"
         "legacy_screen_common.cpp"
         "legacy_screen_main.cpp"
         "legacy_screen_2.cpp"
    INCLUDE_DIRS "."
    REQUIRES freertos bacnet_app product_config lvgl__lvgl sen54
    PRIV_REQUIRES driver esp_driver_i2c esp_driver_spi esp_driver_uart esp_lcd esp_timer
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_CONF_INCLUDE_SIMPLE)
