#include "screen_main.h"

#include <stdio.h>
#include <string.h>

#include "screen_common.h"

static lv_obj_t *s_value_labels[4] = { NULL };
static lv_obj_t *s_footer_label = NULL;
static char s_last_footer_text[40] = {0};

lv_obj_t *screen_main_create(void)
{
    static const char *labels[4] = { "Temp", "%HR", "PM2.5", "VOC" };
    const lv_coord_t disp_text_x = UI_SCREEN_MARGIN_X + 6;
    const lv_coord_t disp_text_y = 20; // distance from top border to the top edge of the text
    const lv_coord_t disp_row_spacing = 25; // spacing between rows of text
    const lv_coord_t value_x = 123; // distance from left border to the left edge of the value
    const lv_coord_t value_y = 18; // distance from top border to the top edge of the value
    const lv_coord_t footer_x = disp_text_x; // distance from left border to the left edge of the footer text
    const lv_coord_t footer_y = 126; // distance from top border to the top edge of the footer text

    lv_obj_t *screen = lv_obj_create(NULL);
    set_screen_bg(screen);
    set_screen_border(screen);

    for (int i = 0; i < 4; ++i) {
        create_text_label(
            screen,
            labels[i],
            disp_text_x,
            disp_text_y + (i * disp_row_spacing),
            &lv_font_montserrat_20,
            lv_color_hex(0xFFFF00)); // YELLOW

        s_value_labels[i] = create_text_label(
            screen,
            "--",
            value_x,
            value_y + (i * disp_row_spacing),
            &lv_font_montserrat_24,
            lv_color_hex(0xFFFFFF)); // WHITE
    }

    create_nav_arrow(screen, ">", LV_ALIGN_RIGHT_MID);


    s_footer_label = create_text_label(
        screen,
        "",
        footer_x,
        footer_y,
        &lv_font_montserrat_20,
        lv_color_hex(0x3A7DFF)); // BLUE

    s_last_footer_text[0] = '\0';
    return screen;
}

void screen_main_update_values(float av1, float av2, float av3, float av4)
{
    char buf[16];
    const float values[4] = { av1, av2, av3, av4 };
    const char *formats[4] = { "%.1f", "%.1f", "%.0f", "%.0f" };

    for (int i = 0; i < 4; ++i) {
        if (!s_value_labels[i]) {
            continue;
        }
        snprintf(buf, sizeof(buf), formats[i], values[i]);
        lv_label_set_text(s_value_labels[i], buf);
    }
}

void screen_main_update_footer(
    unsigned int bacnet_device_id,
    unsigned int mstp_mac_address)
{
    char footer_text[40];

    snprintf(
        footer_text,
        sizeof(footer_text),
        "ID:%u MAC:%u",
        bacnet_device_id,
        mstp_mac_address);

    if (strcmp(footer_text, s_last_footer_text) == 0) {
        return;
    }

    strncpy(s_last_footer_text, footer_text, sizeof(s_last_footer_text) - 1);
    s_last_footer_text[sizeof(s_last_footer_text) - 1] = '\0';
    if (s_footer_label) {
        lv_label_set_text(s_footer_label, s_last_footer_text);
    }
}

void screen_main_reset_footer_cache(void)
{
    s_last_footer_text[0] = '\0';
}
