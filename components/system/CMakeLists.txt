idf_component_register(
    SRCS "app_system.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash product_config
)
