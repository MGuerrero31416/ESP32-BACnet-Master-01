idf_component_register(
    SRCS "app_system.c" "../../main/User_Settings.c"
    INCLUDE_DIRS "." "../../main"
    REQUIRES nvs_flash bacnet-stack
)
