#ifndef SCREEN_3_H
#define SCREEN_3_H

#include <stdbool.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_3_create(void);
void screen_3_update_diagnostics(
    bool fan_failure,
    bool laser_error,
    bool voc_error,
    bool rht_error);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_3_H */
