/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 *
 * CH32F20x embedded flash driver (EFL).
 *
 * CH32F203 uses an STM32F1-compatible FPEC:
 *   FLASH->KEYR  unlock (KEY1/KEY2)
 *   FLASH->CTLR  PG / PER / MER / STRT / LOCK
 *   FLASH->STATR BSY / PGERR / WRPRTERR / EOP
 * Pages are 4KB, programming granularity is a half-word (16 bits).
 */

#include <string.h>
#include "hal.h"

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

#define FLASH_KEY1 0x45670123U
#define FLASH_KEY2 0xCDEF89ABU
#define FLASH_STATR_WR_BSY 0x02U

EFlashDriver EFLD1;

static const flash_descriptor_t efl_lld_descriptor = {
  .attributes    = FLASH_ATTR_MEMORY_MAPPED | FLASH_ATTR_ERASED_IS_ONE,
  .page_size     = CH32F20X_FLASH_PAGE_SIZE,
  .sectors_count = CH32F20X_FLASH_SECTORS_COUNT,
  .sectors       = NULL,
  .sectors_size  = CH32F20X_FLASH_SECTOR_SIZE,
  .address       = (uint8_t *)CH32F20X_FLASH_BASE,
  .size          = CH32F20X_FLASH_SIZE,
};

/* All addresses passed to the helpers below are flash offsets relative to
 * CH32F20X_FLASH_BASE, matching the ChibiOS EFL API. */
static bool flash_range_valid(uint32_t address, uint32_t length) {
  return address <= CH32F20X_FLASH_SIZE && length <= CH32F20X_FLASH_SIZE - address;
}

static bool flash_wait_ready(void) {
  uint32_t timeout = 1000000U;

  while ((FLASH->STATR & FLASH_STATR_BSY) != 0U) {
    if (--timeout == 0U) {
      return false;
    }
  }
  return true;
}

static void flash_clear_flags(void) {
  /* STATR error/EOP bits are write-1-to-clear. */
  FLASH->STATR = FLASH_STATR_EOP | FLASH_STATR_PGERR | FLASH_STATR_WRPRTERR;
}

static void flash_unlock(void) {
  if ((FLASH->CTLR & FLASH_CTLR_LOCK) != 0U) {
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
  }
}

static void flash_unlock_fast(void) {
  flash_unlock();
  FLASH->MODEKEYR = FLASH_KEY1;
  FLASH->MODEKEYR = FLASH_KEY2;
}

static void flash_lock(void) {
  FLASH->CTLR |= FLASH_CTLR_LOCK;
}

static void flash_lock_fast(void) {
  FLASH->CTLR |= FLASH_CTLR_FAST_LOCK;
  flash_lock();
}

static bool flash_erase_page(uint32_t address) {
  bool ok;

  if ((address & (CH32F20X_FLASH_SECTOR_SIZE - 1U)) != 0U || !flash_range_valid(address, CH32F20X_FLASH_SECTOR_SIZE)) {
    return false;
  }

  flash_clear_flags();
  FLASH->CTLR |= FLASH_CTLR_PER;
  FLASH->ADDR = CH32F20X_FLASH_BASE + address;
  FLASH->CTLR |= FLASH_CTLR_STRT;
  ok = flash_wait_ready();
  FLASH->CTLR &= ~FLASH_CTLR_PER;

  if (!ok) {
    return false;
  }
  return (FLASH->STATR & (FLASH_STATR_PGERR | FLASH_STATR_WRPRTERR)) == 0U;
}

static bool flash_program_halfword(uint32_t address, uint16_t data) {
  bool ok;

  if ((address & (CH32F20X_FLASH_PAGE_SIZE - 1U)) != 0U || !flash_range_valid(address, CH32F20X_FLASH_PAGE_SIZE)) {
    return false;
  }

  flash_clear_flags();
  FLASH->CTLR |= FLASH_CTLR_PG;
  *((volatile uint16_t *)(uintptr_t)(CH32F20X_FLASH_BASE + address)) = data;
  ok                              = flash_wait_ready();
  FLASH->CTLR &= ~FLASH_CTLR_PG;

  if (!ok) {
    return false;
  }
  return (FLASH->STATR & (FLASH_STATR_PGERR | FLASH_STATR_WRPRTERR)) == 0U;
}

static bool flash_wait_write_buffer_ready(void) {
  uint32_t timeout = 1000000U;

  while ((FLASH->STATR & FLASH_STATR_WR_BSY) != 0U) {
    if (--timeout == 0U) {
      return false;
    }
  }
  return true;
}

static bool flash_program_page_fast(uint32_t address, const uint8_t *data) {
  bool ok = false;

  if ((address & (EFL_LLD_FAST_PROGRAM_SIZE - 1U)) != 0U || !flash_range_valid(address, EFL_LLD_FAST_PROGRAM_SIZE)) {
    return false;
  }

  flash_clear_flags();
  FLASH->CTLR |= FLASH_CTLR_PAGE_PG;

  if (!flash_wait_ready() || !flash_wait_write_buffer_ready()) {
    goto exit;
  }

  for (size_t i = 0U; i < EFL_LLD_FAST_PROGRAM_SIZE; i += sizeof(uint32_t)) {
    uint32_t word;

    memcpy(&word, data + i, sizeof(word));
    *((volatile uint32_t *)(uintptr_t)(CH32F20X_FLASH_BASE + address + i)) = word;
    if (!flash_wait_write_buffer_ready()) {
      goto exit;
    }
  }

  FLASH->CTLR |= FLASH_CTLR_PG_STRT;
  ok = flash_wait_ready();

exit:
  FLASH->CTLR &= ~FLASH_CTLR_PAGE_PG;
  return ok && (FLASH->STATR & (FLASH_STATR_PGERR | FLASH_STATR_WRPRTERR)) == 0U;
}

void efl_lld_init(void) {
  eflObjectInit(&EFLD1);
}

void efl_lld_start(EFlashDriver *eflp) {
  (void)eflp;
}

void efl_lld_stop(EFlashDriver *eflp) {
  (void)eflp;
}

const flash_descriptor_t *efl_lld_get_descriptor(void *instance) {
  (void)instance;
  return &efl_lld_descriptor;
}

flash_error_t efl_lld_read(void *instance, flash_offset_t offset, size_t n, uint8_t *rp) {
  EFlashDriver *devp = (EFlashDriver *)instance;

  osalDbgCheck(instance != NULL && rp != NULL && n > 0U);
  osalDbgCheck(offset <= CH32F20X_FLASH_SIZE && n <= CH32F20X_FLASH_SIZE - offset);
  osalDbgAssert(devp->state == FLASH_READY, "invalid state");

  if (devp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }

  devp->state = FLASH_READ;
  memcpy(rp, (const void *)(uintptr_t)(CH32F20X_FLASH_BASE + offset), n);
  devp->state = FLASH_READY;
  return FLASH_NO_ERROR;
}

flash_error_t efl_lld_program(void *instance, flash_offset_t offset, size_t n, const uint8_t *pp) {
  EFlashDriver *devp = (EFlashDriver *)instance;

  osalDbgCheck(instance != NULL && pp != NULL && n > 0U);
  osalDbgCheck(offset <= CH32F20X_FLASH_SIZE && n <= CH32F20X_FLASH_SIZE - offset);
  osalDbgAssert(devp->state == FLASH_READY, "invalid state");

  if (devp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  if ((offset & (CH32F20X_FLASH_PAGE_SIZE - 1U)) != 0U || (n & (CH32F20X_FLASH_PAGE_SIZE - 1U)) != 0U) {
    return FLASH_ERROR_PROGRAM;
  }

  const bool use_fast_program = (offset & (EFL_LLD_FAST_PROGRAM_SIZE - 1U)) == 0U &&
                                (n & (EFL_LLD_FAST_PROGRAM_SIZE - 1U)) == 0U &&
                                n >= EFL_LLD_FAST_PROGRAM_SIZE;

  devp->state = FLASH_PGM;
  syssts_t sts = osalSysGetStatusAndLockX();
  if (use_fast_program) {
    flash_unlock_fast();
    for (size_t i = 0U; i < n; i += EFL_LLD_FAST_PROGRAM_SIZE) {
      if (!flash_program_page_fast(offset + i, pp + i)) {
        flash_lock_fast();
        osalSysRestoreStatusX(sts);
        devp->state = FLASH_READY;
        return FLASH_ERROR_PROGRAM;
      }
    }
    flash_lock_fast();
  } else {
    flash_unlock();
    for (size_t i = 0U; i < n; i += CH32F20X_FLASH_PAGE_SIZE) {
      uint16_t halfword;
      memcpy(&halfword, pp + i, sizeof(halfword));
      if (!flash_program_halfword(offset + i, halfword)) {
        flash_lock();
        osalSysRestoreStatusX(sts);
        devp->state = FLASH_READY;
        return FLASH_ERROR_PROGRAM;
      }
    }
    flash_lock();
  }
  osalSysRestoreStatusX(sts);

  devp->state = FLASH_READY;
  return FLASH_NO_ERROR;
}

flash_error_t efl_lld_start_erase_all(void *instance) {
  (void)instance;
  return FLASH_ERROR_UNIMPLEMENTED;
}

flash_error_t efl_lld_start_erase_sector(void *instance, flash_sector_t sector) {
  EFlashDriver *devp = (EFlashDriver *)instance;

  osalDbgCheck(instance != NULL);
  osalDbgCheck(sector < CH32F20X_FLASH_SECTORS_COUNT);
  osalDbgAssert(devp->state == FLASH_READY, "invalid state");

  if (devp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }

  devp->state = FLASH_ERASE;
  syssts_t sts = osalSysGetStatusAndLockX();
  flash_unlock();
  bool success = flash_erase_page(sector * CH32F20X_FLASH_SECTOR_SIZE);
  flash_lock();
  osalSysRestoreStatusX(sts);
  devp->state = FLASH_READY;

  return success ? FLASH_NO_ERROR : FLASH_ERROR_ERASE;
}

flash_error_t efl_lld_query_erase(void *instance, uint32_t *wait_time) {
  EFlashDriver *devp = (EFlashDriver *)instance;

  if (wait_time != NULL) {
    *wait_time = 0;
  }
  if (devp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  return FLASH_NO_ERROR;
}

flash_error_t efl_lld_verify_erase(void *instance, flash_sector_t sector) {
  EFlashDriver *devp = (EFlashDriver *)instance;

  osalDbgCheck(instance != NULL);
  osalDbgCheck(sector < CH32F20X_FLASH_SECTORS_COUNT);
  osalDbgAssert(devp->state == FLASH_READY, "invalid state");

  devp->state            = FLASH_READ;
  const uint32_t *address = (const uint32_t *)(uintptr_t)(CH32F20X_FLASH_BASE + sector * CH32F20X_FLASH_SECTOR_SIZE);
  for (size_t i = 0; i < CH32F20X_FLASH_SECTOR_SIZE / sizeof(uint32_t); i++) {
    /* CH32F20x returns this documented pattern after a successful erase. */
    if (address[i] != ((uint32_t)EFL_LLD_ERASED_VALUE | ((uint32_t)EFL_LLD_ERASED_VALUE << 16U))) {
      devp->state = FLASH_READY;
      return FLASH_ERROR_VERIFY;
    }
  }
  devp->state = FLASH_READY;
  return FLASH_NO_ERROR;
}

#endif /* HAL_USE_EFL == TRUE */
