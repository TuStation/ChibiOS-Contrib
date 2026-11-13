/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file    SYSTICKv1/hal_st_lld.c
 * @brief   CH32F20x system timer driver.
 *          Uses ARM Cortex-M3 SysTick for the ChibiOS system tick.
 */

#include "hal.h"

#if OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC

_Static_assert(CH32F20x_AHB_PRESCALER != 0UL,
               "invalid CH32F20x AHB prescaler");

#define SYSTICK_CK CH32F20x_HCLK

#if !OSAL_IRQ_IS_VALID_PRIORITY(CH32F20x_ST_IRQ_PRIORITY)
#error "Invalid CH32F20x SysTick interrupt priority"
#endif

OSAL_IRQ_HANDLER(SysTick_Handler) {
    OSAL_IRQ_PROLOGUE();
    osalSysLockFromISR();
    osalOsTimerHandlerI();
    osalSysUnlockFromISR();
    OSAL_IRQ_EPILOGUE();
}

void st_lld_init(void) {
    osalDbgAssert((SYSTICK_CK % OSAL_ST_FREQUENCY) == 0UL,
                  "clock rounding error");
    osalDbgAssert((SYSTICK_CK / OSAL_ST_FREQUENCY) <= 0x1000000UL,
                  "clock reload overflow");

    SysTick->LOAD = (SYSTICK_CK / OSAL_ST_FREQUENCY) - 1U;
    SysTick->VAL = 0U;
    nvicSetSystemHandlerPriority(HANDLER_SYSTICK, CH32F20x_ST_IRQ_PRIORITY);
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

#elif OSAL_ST_MODE != OSAL_ST_MODE_NONE
#error "CH32F20x ST driver supports periodic mode only"
#endif
