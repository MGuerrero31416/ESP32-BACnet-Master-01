#include "screen_2.h"

#include "screen_common.h"

lv_obj_t *screen_2_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    set_screen_bg(screen);
    set_screen_border(screen);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "SCREEN-2");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFF3030), 0); // RED
    lv_obj_center(label);

    lv_obj_t *arrow = create_text_label(
    screen,
    "<",
    0,
    0,
    &lv_font_montserrat_24,
    lv_color_hex(0x00FFFF));

lv_obj_align(arrow, LV_ALIGN_LEFT_MID, 5, 0);

    return screen;
}
