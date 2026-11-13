/*
    Copyright 2026 TuStation
    SPDX-License-Identifier: Apache-2.0
*/

#include "ch.h"
#include "hal.h"

int main(void) {
    halInit();
    chSysInit();

    while (true) {
        chThdSleepMilliseconds(1000);
    }
}
