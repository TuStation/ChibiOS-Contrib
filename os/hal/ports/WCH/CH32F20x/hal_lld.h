/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file    hal_lld.h
 * @brief   CH32F20x HAL low-level driver header — clock init.
 */

#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

#define PLATFORM_NAME "WCH CH32F203"

#include "ch32f20x.h"

#define CH32F20x_HSICLK 8000000UL

#if CH32F20x_HSE_ENABLED && (CH32F20x_HSECLK != CH32F20x_HSICLK)
#error "CH32F20x HSE and HSI frequencies must match"
#endif

#define CH32F20x_SYSCLK (CH32F20x_HSICLK * CH32F20x_PLLMUL_VALUE)

#if CH32F20x_SYSCLK != 96000000UL
#error "CH32F20x clock configuration must produce 96 MHz"
#endif

#define CH32F20x_AHB_PRESCALER                                                \
  ((CH32F20x_HPRE == RCC_HPRE_DIV1) ? 1UL :                                  \
   (CH32F20x_HPRE == RCC_HPRE_DIV2) ? 2UL :                                  \
   (CH32F20x_HPRE == RCC_HPRE_DIV4) ? 4UL :                                  \
   (CH32F20x_HPRE == RCC_HPRE_DIV8) ? 8UL :                                  \
   (CH32F20x_HPRE == RCC_HPRE_DIV16) ? 16UL :                                \
   (CH32F20x_HPRE == RCC_HPRE_DIV64) ? 64UL :                                \
   (CH32F20x_HPRE == RCC_HPRE_DIV128) ? 128UL :                              \
   (CH32F20x_HPRE == RCC_HPRE_DIV256) ? 256UL :                              \
   (CH32F20x_HPRE == RCC_HPRE_DIV512) ? 512UL : 0UL)

#define CH32F20x_HCLK                                                         \
  ((CH32F20x_AHB_PRESCALER != 0UL) ?                                          \
   (CH32F20x_SYSCLK / CH32F20x_AHB_PRESCALER) : 0UL)

/**
 * @brief   Returns the frequency of a clock point in Hz.
 * @note    Static implementation.
 *
 * @param[in] clkpt     clock point to be returned
 * @return              The clock point frequency in Hz or zero if the
 *                      frequency is unknown.
 *
 * @notapi
 */
#define hal_lld_get_clock_point(clkpt) 0U

#include "nvic.h"

#ifdef __cplusplus
extern "C" {
#endif
void ch32f20x_clock_init(void);
void hal_lld_init(void);
#ifdef __cplusplus
}
#endif

#endif /* _HAL_LLD_H_ */
