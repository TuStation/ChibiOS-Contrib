/*
    Copyright 2026 TuStation
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef BOARD_H
#define BOARD_H

#define BOARD_NAME "Generic WCH CH32F203 board"
#define CH32F20x_HSECLK 8000000UL

#include "ch32f20x.h"

#ifdef __cplusplus
extern "C" {
#endif
void boardInit(void);
#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
