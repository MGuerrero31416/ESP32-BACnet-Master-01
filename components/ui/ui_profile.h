#ifndef UI_PROFILE_H
#define UI_PROFILE_H

/* Active UI profile selection. */
#define UI_PROFILE_SEN54_320X170 1

#if UI_PROFILE_SEN54_320X170
#include "ui_profile_sen54_320x170.h"
#else
#error "No active UI profile selected in ui_profile.h"
#endif

#endif /* UI_PROFILE_H */
