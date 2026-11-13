/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    GPIOv1/hal_pal_lld.c
 * @brief   CH32F20x PAL low level driver code.
 *
 * @addtogroup PAL
 * @{
 */

#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   CH32 I/O ports configuration.
 * @details Enables the GPIOA, GPIOB, GPIOC, GPIOD and AFIO clocks.
 *
 * @param[in] config    the CH32 ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {
    (void)config;

    /* Enable all GPIO ports and the AFIO interface. */
    RCC->APB2PCENR |= RCC_IOPAEN | RCC_IOPBEN | RCC_IOPCEN | RCC_IOPDEN |
                      RCC_AFIOEN;
}

/**
 * @brief   Configures a group of pads with the same mode.
 * @details Programs the selected nibbles in CFGLR and CFGHR while preserving
 *          all unselected pads.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as push-pull output at 2MHz.
 * @note    Pull-up and pull-down modes also program the output latch because
 *          the latched value selects the resistor direction.
 *
 * @param[in] port      the port identifier
 * @param[in] mask      the group mask
 * @param[in] mode      the mode
 *
 * @notapi
 */
void _pal_lld_setgroupmode(ioportid_t port, ioportmask_t mask, iomode_t mode) {
    static const uint8_t cfgtab[] = {
        4,  /* PAL_MODE_RESET, implemented as input. */
        2,  /* PAL_MODE_UNCONNECTED, output push-pull 2MHz. */
        4,  /* PAL_MODE_INPUT. */
        8,  /* PAL_MODE_INPUT_PULLUP. */
        8,  /* PAL_MODE_INPUT_PULLDOWN. */
        0,  /* PAL_MODE_INPUT_ANALOG. */
        3,  /* PAL_MODE_OUTPUT_PUSHPULL, output push-pull 50MHz. */
        7,  /* PAL_MODE_OUTPUT_OPENDRAIN, output open-drain 50MHz. */
        8,  /* Reserved. */
        8,  /* Reserved. */
        8,  /* Reserved. */
        8,  /* Reserved. */
        8,  /* Reserved. */
        8,  /* Reserved. */
        8,  /* Reserved. */
        8,  /* Reserved. */
        0xB, /* PAL_MODE_CH32_ALTERNATE_PUSHPULL, 50MHz. */
        0xF, /* PAL_MODE_CH32_ALTERNATE_OPENDRAIN, 50MHz. */
    };
    uint32_t mask_low = 0xFFFFFFFFU;
    uint32_t mask_high = 0xFFFFFFFFU;
    uint32_t cfg_low = 0U;
    uint32_t cfg_high = 0U;
    uint32_t cfg;
    unsigned i;

    /* Ignore modes that are not represented in the configuration table. */
    if ((size_t)mode >= sizeof(cfgtab)) {
        return;
    }

    /* The output latch selects pull-up or pull-down direction. */
    if (mode == PAL_MODE_INPUT_PULLUP) {
        port->BSHR = mask;
    } else if (mode == PAL_MODE_INPUT_PULLDOWN) {
        port->BCR = mask;
    }

    cfg = cfgtab[mode];

    /* Each pin i occupies bits [4*i+3:4*i] in its configuration register. */
    for (i = 0; i < 8U; i++) {
        const unsigned shift = i * 4U;
        if ((mask & (1U << i)) != 0U) {
            mask_low &= ~(0xFU << shift);
            cfg_low |= cfg << shift;
        }
        if ((mask & (1U << (i + 8U))) != 0U) {
            mask_high &= ~(0xFU << shift);
            cfg_high |= cfg << shift;
        }
    }
    port->CFGLR = (port->CFGLR & mask_low) | cfg_low;
    port->CFGHR = (port->CFGHR & mask_high) | cfg_high;
}

#endif /* HAL_USE_PAL */

/** @} */
