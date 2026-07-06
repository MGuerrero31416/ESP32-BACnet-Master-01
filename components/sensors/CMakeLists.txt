idf_component_register(
    SRCS "sen54_app.c"
    INCLUDE_DIRS "."
    REQUIRES freertos sen54 bacnet-stack bacnet_objects product_config
)
