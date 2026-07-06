#ifndef SCREEN_2_H
#define SCREEN_2_H

#include <stdbool.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_2_create(void);
void screen_2_update_diagnostics(
	bool fan_failure,
	bool laser_error,
	bool voc_error,
	bool rht_error);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_2_H */
