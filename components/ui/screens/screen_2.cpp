#include "screen_2.h"

extern "C" {
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/bi.h"
}

#include "screen_common.h"

typedef struct {
    lv_obj_t *status_badge;
    lv_obj_t *indicator;
} diagnostic_row_t;

static diagnostic_row_t s_rows[4] = {};
static lv_obj_t *s_screen_2 = NULL;
static lv_timer_t *s_diagnostics_refresh_timer = NULL;

static void set_row_state(diagnostic_row_t *row, bool active)
{
    if (!row || !row->status_badge || !row->indicator) {
        return;
    }

    if (active) {
        lv_label_set_text(row->status_badge, "FAULT");
        lv_obj_set_style_bg_color(row->status_badge, lv_color_hex(0xB83B3B), 0);
        lv_obj_set_style_bg_color(row->indicator, lv_color_hex(0xE13E3E), 0);
    } else {
        lv_label_set_text(row->status_badge, "OK");
        lv_obj_set_style_bg_color(row->status_badge, lv_color_hex(0x2E8C4E), 0);
        lv_obj_set_style_bg_color(row->indicator, lv_color_hex(0x41C36A), 0);
    }
}

static void diagnostics_refresh_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!s_screen_2 || lv_scr_act() != s_screen_2) {
        return;
    }

    screen_2_update_diagnostics(
        Binary_Input_Present_Value(1) == BINARY_ACTIVE,
        Binary_Input_Present_Value(2) == BINARY_ACTIVE,
        Binary_Input_Present_Value(3) == BINARY_ACTIVE,
        Binary_Input_Present_Value(4) == BINARY_ACTIVE);
}

lv_obj_t *screen_2_create(void)
{
    static const char *labels[4] = { "Fan Failure", "Laser Error", "VOC Error", "RHT Error" };
    const lv_coord_t rows_top = UI_SCREEN_MARGIN_Y + 8;
    const lv_coord_t row_x = UI_SCREEN_MARGIN_X;
    const lv_coord_t row_w = UI_SCREEN_CONTENT_WIDTH;

    lv_obj_t *screen = lv_obj_create(NULL);
    s_screen_2 = screen;

    set_screen_bg(screen);
    set_screen_border(screen);

    for (int i = 0; i < 4; ++i) {
        const lv_coord_t row_y = rows_top + (i * (UI_ROW_HEIGHT + UI_ROW_GAP));
        lv_obj_t *row = lv_obj_create(screen);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, row_w, UI_ROW_HEIGHT);
        lv_obj_set_pos(row, row_x, row_y);
        style_card(row, lv_color_hex(0x111111), lv_color_hex(0x2B2B2B));

        create_text_label(
            row,
            labels[i],
            12,
            5,
            &lv_font_montserrat_16,
            lv_color_hex(0xF4F4F4));

        s_rows[i].indicator = lv_obj_create(row);
        lv_obj_remove_style_all(s_rows[i].indicator);
        lv_obj_set_size(s_rows[i].indicator, 16, 16);
        lv_obj_set_style_radius(s_rows[i].indicator, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(s_rows[i].indicator, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_rows[i].indicator, 1, 0);
        lv_obj_set_style_border_color(s_rows[i].indicator, lv_color_hex(0x202020), 0);
        lv_obj_align(s_rows[i].indicator, LV_ALIGN_RIGHT_MID, -96, 0);

        s_rows[i].status_badge = lv_label_create(row);
        lv_obj_set_style_text_font(s_rows[i].status_badge, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s_rows[i].status_badge, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(s_rows[i].status_badge, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(s_rows[i].status_badge, 10, 0);
        lv_obj_set_style_pad_left(s_rows[i].status_badge, 8, 0);
        lv_obj_set_style_pad_right(s_rows[i].status_badge, 8, 0);
        lv_obj_set_style_pad_top(s_rows[i].status_badge, 3, 0);
        lv_obj_set_style_pad_bottom(s_rows[i].status_badge, 3, 0);
        lv_obj_align(s_rows[i].status_badge, LV_ALIGN_RIGHT_MID, -10, 0);
    }

    screen_2_update_diagnostics(false, false, false, false);

    if (!s_diagnostics_refresh_timer) {
        s_diagnostics_refresh_timer = lv_timer_create(diagnostics_refresh_timer_cb, 1000, NULL);
    }

    create_nav_arrow(screen, "<", LV_ALIGN_LEFT_MID);
    create_nav_arrow(screen, ">", LV_ALIGN_RIGHT_MID);
    
    return screen;
}

void screen_2_update_diagnostics(
    bool fan_failure,
    bool laser_error,
    bool voc_error,
    bool rht_error)
{
    const bool states[4] = { fan_failure, laser_error, voc_error, rht_error };

    for (int i = 0; i < 4; ++i) {
        set_row_state(&s_rows[i], states[i]);
    }
}
