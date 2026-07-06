#include "screen_3.h"

#include <stdint.h>

extern "C" {
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/bv.h"
}

#include "screen_common.h"

enum {
    SEN54_RESET_BV_INSTANCE = 1,
    SEN54_MEASUREMENT_BV_INSTANCE = 2,
    SEN54_FAN_CLEANING_BV_INSTANCE = 3
};

#define MOMENTARY_BUTTON_COUNT 2

typedef struct {
    uint32_t instance;
    lv_obj_t *button;
    lv_obj_t *label;
    const char *normal_text;
} momentary_button_t;

static lv_obj_t *s_measurement_switch = NULL;
static lv_obj_t *s_measurement_state = NULL;
static bool s_syncing_measurement = false;
static lv_obj_t *s_screen_3 = NULL;
static lv_timer_t *s_controls_refresh_timer = NULL;
static momentary_button_t s_momentary_buttons[MOMENTARY_BUTTON_COUNT] = {};

static int find_momentary_button_index(uint32_t instance)
{
    for (int i = 0; i < MOMENTARY_BUTTON_COUNT; ++i) {
        if (s_momentary_buttons[i].instance == instance) {
            return i;
        }
    }

    return -1;
}

static void set_momentary_button_feedback(uint32_t instance, bool command_sent)
{
    int idx = find_momentary_button_index(instance);

    if (idx < 0) {
        return;
    }

    if (!s_momentary_buttons[idx].button || !s_momentary_buttons[idx].label) {
        return;
    }

    if (command_sent) {
        lv_obj_set_style_bg_color(s_momentary_buttons[idx].button, lv_color_hex(0x0F6C8D), 0);
        lv_obj_set_style_border_color(s_momentary_buttons[idx].button, lv_color_hex(0x42D9FF), 0);
        lv_label_set_text(s_momentary_buttons[idx].label, "Sent");
    } else {
        lv_obj_set_style_bg_color(s_momentary_buttons[idx].button, lv_color_hex(0x1B1B1B), 0);
        lv_obj_set_style_border_color(s_momentary_buttons[idx].button, lv_color_hex(0x4D4D4D), 0);
        lv_label_set_text(s_momentary_buttons[idx].label, s_momentary_buttons[idx].normal_text);
    }
}

static void send_momentary_bv_command(uint32_t instance)
{
    int idx = find_momentary_button_index(instance);

    if (idx < 0) {
        return;
    }

    Binary_Value_Present_Value_Set(instance, BINARY_ACTIVE);
    set_momentary_button_feedback(instance, true);
}

static void update_measurement_widgets(bool enabled)
{
    if (!s_measurement_switch || !s_measurement_state) {
        return;
    }

    s_syncing_measurement = true;
    if (enabled) {
        lv_obj_add_state(s_measurement_switch, LV_STATE_CHECKED);
        lv_label_set_text(s_measurement_state, "ON");
        lv_obj_set_style_text_color(s_measurement_state, lv_color_hex(0x7CFC98), 0);
    } else {
        lv_obj_clear_state(s_measurement_switch, LV_STATE_CHECKED);
        lv_label_set_text(s_measurement_state, "OFF");
        lv_obj_set_style_text_color(s_measurement_state, lv_color_hex(0xFFB266), 0);
    }
    s_syncing_measurement = false;
}

static void command_button_event_cb(lv_event_t *event)
{
    uint32_t instance = (uint32_t)(uintptr_t)lv_event_get_user_data(event);

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    send_momentary_bv_command(instance);
}

static void controls_refresh_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!s_screen_3 || lv_scr_act() != s_screen_3) {
        return;
    }

    update_measurement_widgets(
        Binary_Value_Present_Value(SEN54_MEASUREMENT_BV_INSTANCE) == BINARY_ACTIVE);

    for (int i = 0; i < MOMENTARY_BUTTON_COUNT; ++i) {
        bool command_active =
            Binary_Value_Present_Value(s_momentary_buttons[i].instance) == BINARY_ACTIVE;
        set_momentary_button_feedback(s_momentary_buttons[i].instance, command_active);
    }
}

static void measurement_switch_event_cb(lv_event_t *event)
{
    lv_obj_t *sw = lv_event_get_target(event);
    BACNET_BINARY_PV next_value;

    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED || s_syncing_measurement) {
        return;
    }

    next_value = lv_obj_has_state(sw, LV_STATE_CHECKED) ? BINARY_ACTIVE : BINARY_INACTIVE;
    Binary_Value_Present_Value_Set(SEN54_MEASUREMENT_BV_INSTANCE, next_value);
    update_measurement_widgets(next_value == BINARY_ACTIVE);
}

static lv_obj_t *create_command_button(
    lv_obj_t *parent,
    const char *label,
    lv_coord_t x,
    lv_coord_t y,
    lv_coord_t w,
    uint32_t instance,
    lv_color_t accent)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_t *button_label = NULL;
    int idx = find_momentary_button_index(instance);

    lv_obj_set_size(button, w, 36);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_radius(button, 10, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x1B1B1B), 0);
    lv_obj_set_style_bg_color(button, accent, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x4D4D4D), 0);
    lv_obj_add_event_cb(button, command_button_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)instance);

    button_label = lv_label_create(button);
    lv_label_set_text(button_label, label);
    lv_obj_set_style_text_font(button_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(button_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(button_label);

    if (idx >= 0) {
        s_momentary_buttons[idx].button = button;
        s_momentary_buttons[idx].label = button_label;
        s_momentary_buttons[idx].normal_text = label;
    }

    return button;
}

lv_obj_t *screen_3_create(void)
{
    const lv_coord_t content_top = UI_SCREEN_MARGIN_Y + 10;
    const lv_coord_t card_x = UI_SCREEN_MARGIN_X;
    const lv_coord_t card_y = content_top;
    const lv_coord_t card_w = UI_SCREEN_CONTENT_WIDTH;
    const lv_coord_t card_h = 44;
    const lv_coord_t button_y = card_y + card_h + 10;
    const lv_coord_t button_w = 138;
    const lv_coord_t button_gap = 16;
    const lv_coord_t left_x = UI_SCREEN_MARGIN_X;
    const lv_coord_t right_x = left_x + button_w + button_gap;

    lv_obj_t *screen = lv_obj_create(NULL);
    s_screen_3 = screen;

    s_momentary_buttons[0].instance = SEN54_RESET_BV_INSTANCE;
    s_momentary_buttons[1].instance = SEN54_FAN_CLEANING_BV_INSTANCE;

    for (int i = 0; i < MOMENTARY_BUTTON_COUNT; ++i) {
        s_momentary_buttons[i].button = NULL;
        s_momentary_buttons[i].label = NULL;
        s_momentary_buttons[i].normal_text = "";
    }

    set_screen_bg(screen);
    set_screen_border(screen);

    lv_obj_t *measurement_card = lv_obj_create(screen);
    lv_obj_remove_style_all(measurement_card);
    lv_obj_set_size(measurement_card, card_w, card_h);
    lv_obj_set_pos(measurement_card, card_x, card_y);
    style_card(measurement_card, lv_color_hex(0x111111), lv_color_hex(0x3D6DCC));

    create_text_label(
        measurement_card,
        "Measurement",
        12,
        11,
        &lv_font_montserrat_20,
        lv_color_hex(0xFFFFFF));

    s_measurement_switch = lv_switch_create(measurement_card);
    lv_obj_align(s_measurement_switch, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(s_measurement_switch, measurement_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_measurement_state = lv_label_create(measurement_card);
    lv_obj_set_style_text_font(s_measurement_state, &lv_font_montserrat_20, 0);
    lv_obj_align_to(s_measurement_state, s_measurement_switch, LV_ALIGN_OUT_LEFT_MID, -14, 0);

    create_command_button(
        screen,
        "Reset",
        left_x,
        button_y,
        button_w,
        SEN54_RESET_BV_INSTANCE,
        lv_color_hex(0xC74B50));

    create_command_button(
        screen,
        "Clean Fan",
        right_x,
        button_y,
        button_w,
        SEN54_FAN_CLEANING_BV_INSTANCE,
        lv_color_hex(0x2F9B88));

    update_measurement_widgets(
        Binary_Value_Present_Value(SEN54_MEASUREMENT_BV_INSTANCE) == BINARY_ACTIVE);

    if (!s_controls_refresh_timer) {
        s_controls_refresh_timer = lv_timer_create(controls_refresh_timer_cb, 1000, NULL);
    }

    create_nav_arrow(screen, "<", LV_ALIGN_LEFT_MID);


    return screen;
}

void screen_3_update_measurement(bool enabled)
{
    update_measurement_widgets(enabled);
}
