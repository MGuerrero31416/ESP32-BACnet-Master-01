#ifndef SCREEN_2_H
#define SCREEN_2_H

#include <stdbool.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_2_create(void);
void screen_2_update_measurement(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_2_H */
