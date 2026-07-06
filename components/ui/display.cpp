#include "display.h"
#include "touch_bsp.h"
#include "screens/screen_2.h"
#include "screens/screen_main.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "display";

typedef enum {
    SCREEN_MAIN = 0,
    SCREEN_2 = 1
} display_screen_t;

static SemaphoreHandle_t s_display_mutex = NULL;
static esp_lcd_panel_io_handle_t s_panel_io = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t *s_buf_1 = NULL;
static lv_color_t *s_buf_2 = NULL;
static lv_disp_drv_t s_disp_drv;
static esp_timer_handle_t s_lvgl_tick_timer = NULL;

static lv_obj_t *s_screen_main = NULL;
static lv_obj_t *s_screen_2 = NULL;
static volatile display_screen_t s_active_screen = SCREEN_MAIN;
static bool s_display_ready = false;

#define DISP_WIDTH 320
#define DISP_HEIGHT 170

#define LCD_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_PIN_NUM_MOSI 13
#define LCD_PIN_NUM_CLK 10
#define LCD_PIN_NUM_CS 12
#define LCD_PIN_NUM_DC 11
#define LCD_PIN_NUM_RST 9
#define LCD_PIN_NUM_BK_LIGHT (-1)
#define LCD_H_RES 170
#define LCD_V_RES 320
#define LCD_GAP_X 0
#define LCD_GAP_Y 35
#define LVGL_DRAW_BUF_LINES 20
#define LVGL_TICK_PERIOD_MS 2

static void create_main_screen(void);
static void create_screen_2(void);
static void set_active_screen(display_screen_t next_screen);
static void touch_gesture_task(void *pvParameters);
static void lvgl_task(void *pvParameters);
static esp_err_t display_panel_init(void);

static uint32_t millis_now(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static bool lvgl_flush_ready_cb(
    esp_lcd_panel_io_handle_t panel_io,
    esp_lcd_panel_io_event_data_t *edata,
    void *user_ctx)
{
    (void)panel_io;
    (void)edata;

    lv_disp_drv_t *disp_drv = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_drv);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    esp_err_t err = esp_lcd_panel_draw_bitmap(
        s_panel_handle,
        area->x1,
        area->y1,
        area->x2 + 1,
        area->y2 + 1,
        color_p);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LCD flush failed: %s", esp_err_to_name(err));
        lv_disp_flush_ready(disp_drv);
    }
}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void create_main_screen(void)
{
    s_screen_main = screen_main_create();
}

static void create_screen_2(void)
{
    s_screen_2 = screen_2_create();
}

static esp_err_t display_panel_init(void)
{
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = LCD_PIN_NUM_MOSI;
    bus_cfg.miso_io_num = -1;
    bus_cfg.sclk_io_num = LCD_PIN_NUM_CLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = DISP_WIDTH * LVGL_DRAW_BUF_LINES * sizeof(lv_color_t);
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO), TAG, "spi_bus_initialize failed");

    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = (gpio_num_t)LCD_PIN_NUM_CS;
    io_config.dc_gpio_num = (gpio_num_t)LCD_PIN_NUM_DC;
    io_config.spi_mode = 0;
    io_config.pclk_hz = LCD_PIXEL_CLOCK_HZ;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = lvgl_flush_ready_cb;
    io_config.user_ctx = &s_disp_drv;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &s_panel_io),
        TAG,
        "esp_lcd_new_panel_io_spi failed");

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = (gpio_num_t)LCD_PIN_NUM_RST;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
    panel_config.bits_per_pixel = 16;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(s_panel_io, &panel_config, &s_panel_handle), TAG, "esp_lcd_new_panel_st7789 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel_handle), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel_handle), TAG, "panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_panel_handle, true), TAG, "panel invert failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(s_panel_handle, true), TAG, "panel swap_xy failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(s_panel_handle, true, false), TAG, "panel mirror failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(s_panel_handle, LCD_GAP_X, LCD_GAP_Y), TAG, "panel gap failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel_handle, true), TAG, "panel on failed");

    #if LCD_PIN_NUM_BK_LIGHT >= 0
    {
        gpio_config_t bk_gpio_config = {};
        bk_gpio_config.mode = GPIO_MODE_OUTPUT;
        bk_gpio_config.pin_bit_mask = 1ULL << LCD_PIN_NUM_BK_LIGHT;
        ESP_RETURN_ON_ERROR(gpio_config(&bk_gpio_config), TAG, "backlight gpio config failed");
        gpio_set_level((gpio_num_t)LCD_PIN_NUM_BK_LIGHT, 1);
    }
    #endif

    return ESP_OK;
}

static void set_active_screen(display_screen_t next_screen)
{
    if (s_active_screen == next_screen || !s_display_mutex || !s_display_ready) {
        return;
    }

    if (xSemaphoreTake(s_display_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }

    lv_scr_load(next_screen == SCREEN_MAIN ? s_screen_main : s_screen_2);
    if (next_screen == SCREEN_MAIN) {
        screen_main_reset_footer_cache();
   //     ESP_LOGI(TAG, "Switched to MAIN screen");
    } else {
   //     ESP_LOGI(TAG, "Switched to SCREEN-2");
    }
    s_active_screen = next_screen;

    xSemaphoreGive(s_display_mutex);
}

static void touch_gesture_task(void *pvParameters)
{
    (void)pvParameters;

    bool touch_active = false;
    int16_t start_x = 0;
    int16_t start_y = 0;
    int16_t last_x = 0;
    int16_t last_y = 0;
    uint32_t gesture_start_ms = 0;

    while (1) {
        uint16_t x = 0;
        uint16_t y = 0;
        bool pressed = touch_bsp_get_xy(&x, &y);

        if (pressed) {
            if (!touch_active) {
                touch_active = true;
                start_x = (int16_t)x;
                start_y = (int16_t)y;
                gesture_start_ms = millis_now();
            }
            last_x = (int16_t)x;
            last_y = (int16_t)y;
        } else if (touch_active) {
            int dx = (int)last_x - (int)start_x;
            int dy = (int)last_y - (int)start_y;
            int abs_dx = (dx < 0) ? -dx : dx;
            int abs_dy = (dy < 0) ? -dy : dy;
            int dominant_delta = (abs_dx >= abs_dy) ? dx : dy;
            int orthogonal_delta = (abs_dx >= abs_dy) ? dy : dx;
            uint32_t duration_ms = millis_now() - gesture_start_ms;

            if (duration_ms <= 1200 && ((orthogonal_delta < 0 ? -orthogonal_delta : orthogonal_delta) <= 70)) {
                if (dominant_delta <= -45) {
             //       ESP_LOGI(TAG, "Swipe LEFT detected: dx=%d dy=%d", dx, dy);
                    set_active_screen(SCREEN_2);
                } else if (dominant_delta >= 45) {
             //       ESP_LOGI(TAG, "Swipe RIGHT detected: dx=%d dy=%d", dx, dy);
                    set_active_screen(SCREEN_MAIN);
                }
            }

            touch_active = false;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void lvgl_task(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        uint32_t delay_ms = 10;

        if (s_display_mutex && xSemaphoreTake(s_display_mutex, portMAX_DELAY) == pdTRUE) {
            delay_ms = lv_timer_handler();
            xSemaphoreGive(s_display_mutex);
        }

        if (delay_ms < 5) {
            delay_ms = 5;
        } else if (delay_ms > 20) {
            delay_ms = 20;
        }

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

extern "C" void display_init(void)
{
    s_display_mutex = xSemaphoreCreateMutex();
    if (!s_display_mutex) {
        ESP_LOGE(TAG, "Failed to create display mutex");
        return;
    }

    ESP_ERROR_CHECK(display_panel_init());

    lv_init();

    s_buf_1 = (lv_color_t *)heap_caps_malloc(
        DISP_WIDTH * LVGL_DRAW_BUF_LINES * sizeof(lv_color_t),
        MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_buf_2 = (lv_color_t *)heap_caps_malloc(
        DISP_WIDTH * LVGL_DRAW_BUF_LINES * sizeof(lv_color_t),
        MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_buf_1 || !s_buf_2) {
        ESP_LOGE(TAG, "Failed to allocate LVGL draw buffers");
        return;
    }

    lv_disp_draw_buf_init(&s_draw_buf, s_buf_1, s_buf_2, DISP_WIDTH * LVGL_DRAW_BUF_LINES);
    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = DISP_WIDTH;
    s_disp_drv.ver_res = DISP_HEIGHT;
    s_disp_drv.flush_cb = lvgl_flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&s_disp_drv);

    esp_timer_create_args_t tick_timer_args = {};
    tick_timer_args.callback = lvgl_tick_cb;
    tick_timer_args.name = "lvgl_tick";
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &s_lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000ULL));

    if (xSemaphoreTake(s_display_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        create_main_screen();
        create_screen_2();
        lv_scr_load(s_screen_main);
        s_active_screen = SCREEN_MAIN;
        s_display_ready = true;
        xSemaphoreGive(s_display_mutex);
    }

    esp_err_t touch_err = touch_bsp_init();
    if (touch_err != ESP_OK) {
        ESP_LOGW(TAG, "Touch init failed (addr 0x%02X, SDA=%d, SCL=%d): %s",
                 TOUCH_I2C_ADDR, TOUCH_I2C_SDA_PIN, TOUCH_I2C_SCL_PIN, esp_err_to_name(touch_err));
    } else if (xTaskCreate(touch_gesture_task, "touch_gesture", 4096, NULL, 3, NULL) != pdPASS) {
        ESP_LOGW(TAG, "Failed to start touch gesture task");
    }

    if (xTaskCreate(lvgl_task, "lvgl", 6144, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start LVGL task");
    }

    ESP_LOGI(TAG, "Display initialized with LVGL");
}

extern "C" void display_set_link_status(bool wifi_connected, bool mstp_connected)
{
    (void)wifi_connected;
    (void)mstp_connected;
}

extern "C" void display_update_values(float av1, float av2, float av3, float av4)
{
    if (s_active_screen != SCREEN_MAIN || !s_display_mutex || !s_display_ready) {
        return;
    }
    if (xSemaphoreTake(s_display_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }

    screen_main_update_values(av1, av2, av3, av4);

    xSemaphoreGive(s_display_mutex);
}

extern "C" void display_update_footer(
    unsigned int bacnet_device_id,
    unsigned int mstp_mac_address,
    const char *ip_address)
{
    (void)ip_address;

    if (s_active_screen != SCREEN_MAIN || !s_display_mutex || !s_display_ready) {
        return;
    }

    if (xSemaphoreTake(s_display_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }

    screen_main_update_footer(bacnet_device_id, mstp_mac_address);

    xSemaphoreGive(s_display_mutex);
}
