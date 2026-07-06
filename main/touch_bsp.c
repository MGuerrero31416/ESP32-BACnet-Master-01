#include "touch_bsp.h"

#include "driver/i2c_master.h"

#define TOUCH_I2C_PORT    1
#define TOUCH_I2C_FREQ_HZ 200000

static bool s_touch_ready = false;
static i2c_master_bus_handle_t s_touch_bus = NULL;
static i2c_master_dev_handle_t s_touch_dev = NULL;

esp_err_t touch_bsp_init(void)
{
    esp_err_t err;

    if (s_touch_ready) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t bus_cfg = {
        .i2c_port = TOUCH_I2C_PORT,
        .sda_io_num = TOUCH_I2C_SDA_PIN,
        .scl_io_num = TOUCH_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    err = i2c_new_master_bus(&bus_cfg, &s_touch_bus);
    if (err != ESP_OK) {
        return err;
    }

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOUCH_I2C_ADDR,
        .scl_speed_hz = TOUCH_I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(s_touch_bus, &dev_cfg, &s_touch_dev);
    if (err != ESP_OK) {
        return err;
    }

    // Reference repo writes 0x00 to register 0x00 to switch touch IC into normal mode.
    uint8_t init_cmd[2] = { 0x00, 0x00 };
    err = i2c_master_transmit(s_touch_dev, init_cmd, sizeof(init_cmd), 100);
    if (err != ESP_OK) {
        return err;
    }

    s_touch_ready = true;
    return ESP_OK;
}

bool touch_bsp_get_xy(uint16_t *x, uint16_t *y)
{
    uint8_t reg = 0x00;
    uint8_t tp_temp[7] = {0};
    uint8_t touch_num;
    uint16_t raw_x;
    uint16_t raw_y;

    if (!s_touch_ready || !s_touch_dev || !x || !y) {
        return false;
    }

    if (i2c_master_transmit_receive(s_touch_dev, &reg, 1, tp_temp, sizeof(tp_temp), 100) != ESP_OK) {
        return false;
    }

    touch_num = tp_temp[2] & 0x0F;
    if (!touch_num) {
        return false;
    }

    raw_x = ((uint16_t)(tp_temp[3] & 0x0F) << 8) | (uint16_t)tp_temp[4];
    raw_y = ((uint16_t)(tp_temp[5] & 0x0F) << 8) | (uint16_t)tp_temp[6];

    // Map touch IC coordinates to TFT rotation=1 (320x170 landscape).
    // On this board raw Y is horizontal and raw X is vertical.
    *x = raw_y;
    *y = (raw_x < 170U) ? (169U - raw_x) : 0U;

    if (*x > 319U) {
        *x = 319U;
    }

    return true;
}
