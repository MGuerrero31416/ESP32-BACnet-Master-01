idf_component_register(
    SRCS "mstp_rs485.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio esp_driver_uart esp_timer freertos board_config
)
