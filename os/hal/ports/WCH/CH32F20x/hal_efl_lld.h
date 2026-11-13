/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HAL_EFL_LLD_H
#define HAL_EFL_LLD_H

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

#include "ch32f20x.h"

/* CH32F203CB: 128KB code flash, 4KB sectors, half-word programming. */
#define CH32F20X_FLASH_BASE 0x08000000U
#define CH32F20X_FLASH_SECTOR_SIZE 0x1000U
#define CH32F20X_FLASH_PAGE_SIZE 2U
#define CH32F20X_FLASH_SIZE 0x20000U
#define CH32F20X_FLASH_SECTORS_COUNT (CH32F20X_FLASH_SIZE / CH32F20X_FLASH_SECTOR_SIZE)

/* Fast programming transfers one 256-byte page at a time. */
#define EFL_LLD_FAST_PROGRAM_SIZE 256U

/* Documented readback value of an erased CH32F20x flash half-word. */
#define EFL_LLD_ERASED_VALUE 0xE339U

/* Generic flash size used by the wear-levelling driver's auto-detection. */
#ifndef FLASH_SIZE
#    define FLASH_SIZE CH32F20X_FLASH_SIZE
#endif

#define efl_lld_driver_fields uint32_t dummy;
#define efl_lld_config_fields uint32_t dummy;

#if !defined(__DOXYGEN__)
extern EFlashDriver EFLD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif

void efl_lld_init(void);
void efl_lld_start(EFlashDriver *eflp);
void efl_lld_stop(EFlashDriver *eflp);
const flash_descriptor_t *efl_lld_get_descriptor(void *instance);
flash_error_t efl_lld_read(void *instance, flash_offset_t offset, size_t n, uint8_t *rp);
flash_error_t efl_lld_program(void *instance, flash_offset_t offset, size_t n, const uint8_t *pp);
flash_error_t efl_lld_start_erase_all(void *instance);
flash_error_t efl_lld_start_erase_sector(void *instance, flash_sector_t sector);
flash_error_t efl_lld_query_erase(void *instance, uint32_t *wait_time);
flash_error_t efl_lld_verify_erase(void *instance, flash_sector_t sector);

#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_EFL == TRUE */

#endif /* HAL_EFL_LLD_H */
