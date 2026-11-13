/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file    SYSTICKv1/hal_st_lld.h
 * @brief   CH32F20x system timer driver header.
 *          Uses ARM Cortex-M3 SysTick.
 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

#if !defined(CH32F20x_ST_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define CH32F20x_ST_IRQ_PRIORITY 8
#endif

#ifdef __cplusplus
extern "C" {
#endif
void st_lld_init(void);
#ifdef __cplusplus
}
#endif

#endif /* HAL_ST_LLD_H */
