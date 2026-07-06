#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* Active board profile selection. */
#define BOARD_SPOTPEAR_LCD_1_9 1

#if BOARD_SPOTPEAR_LCD_1_9
#include "board_spotpear_lcd_1_9.h"
#else
#error "No active board profile selected in board_config.h"
#endif

#endif /* BOARD_CONFIG_H */
