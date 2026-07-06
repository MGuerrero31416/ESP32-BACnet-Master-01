#ifndef SCREEN_COMMON_H
#define SCREEN_COMMON_H

#include "lvgl.h"

void set_screen_bg(lv_obj_t *screen);
void set_screen_border(lv_obj_t *screen);
lv_obj_t *create_text_label(
    lv_obj_t *parent,
    const char *text,
    lv_coord_t x,
    lv_coord_t y,
    const lv_font_t *font,
    lv_color_t color);

#endif /* SCREEN_COMMON_H */
