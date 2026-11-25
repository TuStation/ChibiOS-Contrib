/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 *
 * CH32F20x TIM2..TIM4 PWM low level driver.
 */
#include "hal.h"

#if HAL_USE_PWM || defined(__DOXYGEN__)

#if CH32F20x_PWM_USE_TIM2 || defined(__DOXYGEN__)
PWMDriver PWMD2;
#endif
#if CH32F20x_PWM_USE_TIM3 || defined(__DOXYGEN__)
PWMDriver PWMD3;
#endif
#if CH32F20x_PWM_USE_TIM4 || defined(__DOXYGEN__)
PWMDriver PWMD4;
#endif

static uint32_t pwm_timer_clock(void)
{
    static const uint8_t apb1_div[8] = {1, 1, 1, 1, 2, 4, 8, 16};
    uint32_t ppre1 = (RCC->CFGR0 >> 8) & 0x7U;
    uint32_t pclk1 = CH32F20x_HCLK / apb1_div[ppre1];

    return (ppre1 < 4U) ? pclk1 : (pclk1 * 2U);
}

static void pwm_enable_clock(PWMDriver *pwmp)
{
    if (pwmp->tim == TIM2) {
        RCC->APB1PCENR |= RCC_TIM2EN;
    }
    if (pwmp->tim == TIM3) {
        RCC->APB1PCENR |= RCC_TIM3EN;
    }
    if (pwmp->tim == TIM4) {
        RCC->APB1PCENR |= RCC_TIM4EN;
    }
}

static void pwm_disable_clock(PWMDriver *pwmp)
{
    if (pwmp->tim == TIM2) {
        RCC->APB1PCENR &= ~RCC_TIM2EN;
    }
    if (pwmp->tim == TIM3) {
        RCC->APB1PCENR &= ~RCC_TIM3EN;
    }
    if (pwmp->tim == TIM4) {
        RCC->APB1PCENR &= ~RCC_TIM4EN;
    }
}

static void pwm_reset(PWMDriver *pwmp)
{
    uint32_t reset_mask = 0U;

    if (pwmp->tim == TIM2) {
        reset_mask = RCC_TIM2RST;
    }
    if (pwmp->tim == TIM3) {
        reset_mask = RCC_TIM3RST;
    }
    if (pwmp->tim == TIM4) {
        reset_mask = RCC_TIM4RST;
    }

    RCC->APB1PRSTR |= reset_mask;
    RCC->APB1PRSTR &= ~reset_mask;
}

static void pwm_set_compare(TIM_TypeDef *tim, pwmchannel_t channel, pwmcnt_t width)
{
    switch (channel) {
        case 0:
            tim->CH1CVR = width;
            break;
        case 1:
            tim->CH2CVR = width;
            break;
        case 2:
            tim->CH3CVR = width;
            break;
        case 3:
            tim->CH4CVR = width;
            break;
        default:
            break;
    }
}

void pwm_lld_init(void)
{
#if CH32F20x_PWM_USE_TIM2 || defined(__DOXYGEN__)
    pwmObjectInit(&PWMD2);
    PWMD2.channels = PWM_CHANNELS;
    PWMD2.tim = TIM2;
#endif
#if CH32F20x_PWM_USE_TIM3 || defined(__DOXYGEN__)
    pwmObjectInit(&PWMD3);
    PWMD3.channels = PWM_CHANNELS;
    PWMD3.tim = TIM3;
#endif
#if CH32F20x_PWM_USE_TIM4 || defined(__DOXYGEN__)
    pwmObjectInit(&PWMD4);
    PWMD4.channels = PWM_CHANNELS;
    PWMD4.tim = TIM4;
#endif
}

void pwm_lld_start(PWMDriver *pwmp)
{
    uint32_t timer_clock;
    uint32_t divider;

    if (pwmp->state == PWM_STOP) {
        pwm_enable_clock(pwmp);
        pwm_reset(pwmp);
    }

    timer_clock = pwm_timer_clock();
    osalDbgAssert(pwmp->config->frequency != 0U, "invalid frequency");
    osalDbgAssert(pwmp->config->period != 0U, "invalid period");
    divider = timer_clock / pwmp->config->frequency;
    osalDbgAssert((divider >= 1U) && (divider <= 65536U) &&
                  ((divider * pwmp->config->frequency) == timer_clock),
                  "invalid frequency");

    pwmp->period = pwmp->config->period;
    pwmp->tim->CTLR1 = 0U;
    pwmp->tim->CTLR2 = 0U;
    pwmp->tim->SMCFGR = 0U;
    pwmp->tim->DMAINTENR = 0U;
    pwmp->tim->INTFR = 0U;
    pwmp->tim->CHCTLR1 = 0U;
    pwmp->tim->CHCTLR2 = 0U;
    pwmp->tim->CCER = 0U;
    pwmp->tim->CNT = 0U;
    pwmp->tim->CH1CVR = 0U;
    pwmp->tim->CH2CVR = 0U;
    pwmp->tim->CH3CVR = 0U;
    pwmp->tim->CH4CVR = 0U;
    pwmp->tim->PSC = (uint16_t)(divider - 1U);
    pwmp->tim->ATRLR = (uint16_t)(pwmp->period - 1U);
    pwmp->tim->SWEVGR = 1U;
    pwmp->tim->INTFR = 0U;
    pwmp->tim->CTLR1 = TIM_ARPE | TIM_URS | TIM_CEN;
}

void pwm_lld_stop(PWMDriver *pwmp)
{
    if (pwmp->state == PWM_READY) {
        pwmp->tim->CTLR1 = 0U;
        pwmp->tim->DMAINTENR = 0U;
        pwmp->tim->INTFR = 0U;
        pwmp->tim->CCER = 0U;
        pwm_disable_clock(pwmp);
    }
}

void pwm_lld_change_period(PWMDriver *pwmp, pwmcnt_t period)
{
    osalDbgAssert(period != 0U, "invalid period");
    pwmp->period = period;
    pwmp->tim->ATRLR = (uint16_t)(period - 1U);
}

void pwm_lld_enable_channel(PWMDriver *pwmp, pwmchannel_t channel, pwmcnt_t width)
{
    volatile uint16_t *ccmr;
    uint16_t shift;
    uint16_t ccer_shift;
    uint16_t ccer;
    pwmmode_t mode;

    if (channel >= PWM_CHANNELS) {
        return;
    }
    if (width > pwmp->period) {
        width = pwmp->period;
    }
    ccmr = (channel < 2U) ? &pwmp->tim->CHCTLR1 : &pwmp->tim->CHCTLR2;
    shift = (uint16_t)((channel & 1U) * 8U);
    *ccmr = (uint16_t)((*ccmr & ~(0xFFU << shift)) | (0x68U << shift));
    pwm_set_compare(pwmp->tim, channel, width);

    mode = pwmp->config->channels[channel].mode & PWM_OUTPUT_MASK;
    ccer_shift = (uint16_t)(channel * 4U);
    ccer = pwmp->tim->CCER & (uint16_t)~(0xFU << ccer_shift);
    if (mode == PWM_OUTPUT_ACTIVE_LOW) {
        ccer |= (uint16_t)(3U << ccer_shift);
    }
    else if (mode == PWM_OUTPUT_ACTIVE_HIGH) {
        ccer |= (uint16_t)(1U << ccer_shift);
    }
    pwmp->tim->CCER = ccer;
}

void pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t channel)
{
    if (channel < PWM_CHANNELS) {
        pwm_set_compare(pwmp->tim, channel, 0U);
    }
}

void pwm_lld_enable_periodic_notification(PWMDriver *pwmp)
{
    (void)pwmp;
}

void pwm_lld_disable_periodic_notification(PWMDriver *pwmp)
{
    (void)pwmp;
}

void pwm_lld_enable_channel_notification(PWMDriver *pwmp, pwmchannel_t channel)
{
    (void)pwmp;
    (void)channel;
}

void pwm_lld_disable_channel_notification(PWMDriver *pwmp, pwmchannel_t channel)
{
    (void)pwmp;
    (void)channel;
}

#endif
