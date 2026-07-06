#include "screen_common.h"

void set_screen_bg(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0); // BLACK
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
}

void set_screen_border(lv_obj_t *screen)
{
    lv_obj_set_style_border_width(screen, 2, 0);
    lv_obj_set_style_border_color(screen, lv_color_hex(0xFF00FF), 0); // MAGENTA
    lv_obj_set_style_pad_all(screen, 0, 0);
}

lv_obj_t *create_text_label(
    lv_obj_t *parent,
    const char *text,
    lv_coord_t x,
    lv_coord_t y,
    const lv_font_t *font,
    lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

lv_obj_t *create_screen_title(lv_obj_t *parent, const char *title)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, UI_HEADER_Y);
    return label;
}

lv_obj_t *create_nav_arrow(lv_obj_t *parent, const char *text, lv_align_t align)
{
    lv_obj_t *arrow = lv_label_create(parent);
    lv_label_set_text(arrow, text);
    lv_obj_set_style_text_font(arrow, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(arrow, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_bg_opa(arrow, LV_OPA_TRANSP, 0);
    lv_obj_align(arrow, align, 0, 0);

    if (align == LV_ALIGN_LEFT_MID) {
        lv_obj_set_x(arrow, UI_NAV_ARROW_INSET);
    } else if (align == LV_ALIGN_RIGHT_MID) {
        lv_obj_set_x(arrow, -UI_NAV_ARROW_INSET);
    }

    return arrow;
}

void style_card(lv_obj_t *obj, lv_color_t bg_color, lv_color_t border_color)
{
    lv_obj_set_style_radius(obj, UI_CARD_RADIUS, 0);
    lv_obj_set_style_bg_color(obj, bg_color, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, border_color, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}
