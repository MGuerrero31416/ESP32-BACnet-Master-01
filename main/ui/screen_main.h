#ifndef SCREEN_MAIN_H
#define SCREEN_MAIN_H

#include "lvgl.h"

lv_obj_t *screen_main_create(void);
void screen_main_update_values(float av1, float av2, float av3, float av4);
void screen_main_update_footer(
    unsigned int bacnet_device_id,
    unsigned int mstp_mac_address);
void screen_main_reset_footer_cache(void);

#endif /* SCREEN_MAIN_H */
