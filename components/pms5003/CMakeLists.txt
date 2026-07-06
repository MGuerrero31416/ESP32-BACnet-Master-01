idf_component_register(
    SRCS "pms5003.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_driver_uart esp_driver_gpio freertos board_config
)





