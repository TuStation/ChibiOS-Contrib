/*
    Copyright 2026 TuStation
    SPDX-License-Identifier: Apache-2.0
*/

#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)
const PALConfig pal_default_config = {0};
#endif

void __early_init(void) {
    ch32f20x_clock_init();
}

void boardInit(void) {
}
