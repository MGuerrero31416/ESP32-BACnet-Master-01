idf_component_register(
    SRCS "analog_input.c" "analog_value.c" "binary_input.c" "binary_value.c" "binary_output.c"
    INCLUDE_DIRS "."
    REQUIRES bacnet-stack product_config nvs_flash freertos
)
