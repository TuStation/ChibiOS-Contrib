/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file    hal_lld.c
 * @brief   CH32F20x HAL low-level driver.
 *
 * Clock tree: HSE 8MHz or HSI 8MHz through PLL x12 = 96MHz.
 * SYSCLK = 96MHz, AHB = 96MHz, APB2 = 96MHz, APB1 = 48MHz.
 *
 * The USB controller needs an exact 48MHz clock.  The PLL output is divided
 * by two through CFGR0.USBPRE[1:0] = 01.
 */

#include "hal.h"

#if CH32F20x_PLLMUL_VALUE == 18
#define CH32F20x_PLLMUL RCC_PLLMULL18
#elif (CH32F20x_PLLMUL_VALUE >= 2) && (CH32F20x_PLLMUL_VALUE <= 16)
#define CH32F20x_PLLMUL ((CH32F20x_PLLMUL_VALUE - 2UL) << 18)
#else
#error "CH32F20x PLL multiplier must be 2 through 16, or 18"
#endif

_Static_assert(CH32F20x_SW == RCC_SW_PLL,
               "CH32F20x clock configuration currently requires PLL as SYSCLK");

void ch32f20x_clock_init(void) {
    if (CH32F20x_NO_INIT) {
        return;
    }

    bool use_hse = false;

#if CH32F20x_HSE_ENABLED
    /* Prefer the external 8MHz crystal when it is available. */
    RCC->CTLR &= ~RCC_HSEBYP;
    RCC->CTLR |= RCC_HSEON;
    uint32_t hse_timeout = 0x10000U;
    while (((RCC->CTLR & RCC_HSERDY) == 0U) && (--hse_timeout != 0U)) {
    }
    use_hse = (RCC->CTLR & RCC_HSERDY) != 0U;
#endif

    if (use_hse) {
        EXTEN->EXTEN_CTR &= ~EXTEN_PLL_HSI_PRE;
    } else {
#if CH32F20x_HSE_ENABLED
        RCC->CTLR &= ~RCC_HSEON;
#endif
        /* Fall back to the internal 8MHz oscillator at the same frequency. */
        RCC->CTLR |= RCC_HSION;
        while ((RCC->CTLR & RCC_HSIRDY) == 0U) {
        }
        EXTEN->EXTEN_CTR |= EXTEN_PLL_HSI_PRE;
    }

    /* PLL input is either HSE or HSI, both produce 96MHz at x12. */
    uint32_t pll_source = use_hse
                            ? (RCC_PLLSRC_HSE | RCC_PLLXTPRE_HSE)
                            : RCC_PLLSRC_HSI_Div2;
    RCC->CFGR0 = (RCC->CFGR0 & ~(RCC_PLLSRC | RCC_PLLXTPRE | RCC_PLLMULL)) |
                 pll_source | CH32F20x_PLLMUL;

    /* Bus and ADC prescalers from mcuconf.h. */
    RCC->CFGR0 = (RCC->CFGR0 & ~(RCC_HPRE | RCC_PPRE1 | RCC_PPRE2 | RCC_ADCPRE)) |
                 CH32F20x_HPRE | CH32F20x_PPRE1 | CH32F20x_PPRE2 |
                 CH32F20x_ADCPRE;

    /* USB clock prescaler from mcuconf.h. */
    RCC->CFGR0 = (RCC->CFGR0 & ~(3U << 22)) | CH32F20x_USBPRE;

    /* Enable PLL */
    RCC->CTLR |= RCC_PLLON;
    while ((RCC->CTLR & RCC_PLLRDY) == 0U) {
    }

    /* Switch SYSCLK to PLL */
    RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | CH32F20x_SW;
    while ((RCC->CFGR0 & RCC_SWS) != RCC_SWS_PLL) {
    }
}

void hal_lld_init(void) {
    /* Reset all APB1 and APB2 peripherals. */
    RCC->APB1PRSTR = 0xFFFFFFFFU;
    RCC->APB2PRSTR = 0xFFFFFFFFU;
    RCC->APB1PRSTR = 0U;
    RCC->APB2PRSTR = 0U;
}
