# Legacy/inactive component note:
# This directory is kept for reference but is not part of the active build
# because components/peripherals is not registered as a component root.
idf_component_register(
    SRCS "pms5003.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)
