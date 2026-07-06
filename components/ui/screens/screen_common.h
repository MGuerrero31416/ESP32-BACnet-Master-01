#ifndef SCREEN_COMMON_H
#define SCREEN_COMMON_H

#include "lvgl.h"
#include "ui_profile.h"

#define UI_SCREEN_WIDTH UI_PROFILE_EXPECTED_DISPLAY_WIDTH
#define UI_SCREEN_HEIGHT UI_PROFILE_EXPECTED_DISPLAY_HEIGHT

#define UI_SCREEN_MARGIN_X 14
#define UI_SCREEN_MARGIN_Y 10
#define UI_SCREEN_CONTENT_WIDTH (UI_SCREEN_WIDTH - (UI_SCREEN_MARGIN_X * 2))

#define UI_HEADER_HEIGHT 28
#define UI_HEADER_Y UI_SCREEN_MARGIN_Y
#define UI_SUBTITLE_Y (UI_HEADER_Y + 24)
#define UI_NAV_ARROW_INSET 8

#define UI_ROW_HEIGHT 28
#define UI_ROW_GAP 8
#define UI_CARD_RADIUS 10

void set_screen_bg(lv_obj_t *screen);
void set_screen_border(lv_obj_t *screen);
lv_obj_t *create_text_label(
    lv_obj_t *parent,
    const char *text,
    lv_coord_t x,
    lv_coord_t y,
    const lv_font_t *font,
    lv_color_t color);
lv_obj_t *create_screen_title(lv_obj_t *parent, const char *title);
lv_obj_t *create_nav_arrow(lv_obj_t *parent, const char *text, lv_align_t align);
void style_card(lv_obj_t *obj, lv_color_t bg_color, lv_color_t border_color);

#endif /* SCREEN_COMMON_H */
