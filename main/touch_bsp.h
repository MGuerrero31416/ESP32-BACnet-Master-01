#ifndef TOUCH_BSP_H
#define TOUCH_BSP_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_I2C_SDA_PIN 47
#define TOUCH_I2C_SCL_PIN 48
#define TOUCH_I2C_ADDR    0x15

esp_err_t touch_bsp_init(void);
bool touch_bsp_get_xy(uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif

#endif
