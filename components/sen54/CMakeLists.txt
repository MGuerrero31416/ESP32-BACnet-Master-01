idf_component_register(
    SRCS "sen54.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_driver_i2c freertos
)
