/*
    Copyright 2026 TuStation
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef MCUCONF_H
#define MCUCONF_H

#define CH32F20x_MCUCONF

#define CH32F20x_NO_INIT              FALSE
#define CH32F20x_HSE_ENABLED           TRUE
#define CH32F20x_SW                   RCC_SW_PLL
#define CH32F20x_PLLMUL_VALUE          12
#define CH32F20x_HPRE                 RCC_HPRE_DIV1
#define CH32F20x_PPRE1                RCC_PPRE1_DIV2
#define CH32F20x_PPRE2                RCC_PPRE2_DIV1
#define CH32F20x_ADCPRE               RCC_ADCPRE_DIV8
#define CH32F20x_USBPRE               RCC_USBPRE_DIV2

/* Keep this platform smoke test limited to PAL and USB. */
#define HAL_USE_ADC                    FALSE
#define HAL_USE_CAN                    FALSE
#define HAL_USE_CRY                    FALSE
#define HAL_USE_DAC                    FALSE
#define HAL_USE_EFL                    FALSE
#define HAL_USE_GPT                    FALSE
#define HAL_USE_I2C                    FALSE
#define HAL_USE_I2S                    FALSE
#define HAL_USE_ICU                    FALSE
#define HAL_USE_MAC                    FALSE
#define HAL_USE_MMC_SPI                FALSE
#define HAL_USE_PAL                    TRUE
#define HAL_USE_PWM                    FALSE
#define HAL_USE_QEI                    FALSE
#define HAL_USE_RTC                    FALSE
#define HAL_USE_SDC                    FALSE
#define HAL_USE_SERIAL                 FALSE
#define HAL_USE_SERIAL_USB             FALSE
#define HAL_USE_SIO                    FALSE
#define HAL_USE_SPI                    FALSE
#define HAL_USE_TRNG                   FALSE
#define HAL_USE_UART                   FALSE
#define HAL_USE_USB                    TRUE
#define HAL_USE_WDG                    FALSE
#define HAL_USE_WSPI                   FALSE

#endif /* MCUCONF_H */
