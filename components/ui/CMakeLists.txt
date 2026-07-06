idf_component_register(
    SRCS "ui_manager.c"
         "display.cpp"
         "touch_bsp.c"
         "screens/screen_common.cpp"
         "screens/screen_main.cpp"
         "screens/screen_2.cpp"
         "screens/screen_3.cpp"
    INCLUDE_DIRS "." "screens"
    REQUIRES freertos bacnet_app product_config lvgl__lvgl sen54 board_config
    PRIV_REQUIRES driver esp_driver_i2c esp_driver_spi esp_driver_uart esp_lcd esp_timer
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_CONF_INCLUDE_SIMPLE)
