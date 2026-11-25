/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 *
 * CH32F20x TIM2..TIM4 PWM low level driver.
 */
#ifndef HAL_PWM_LLD_H
#define HAL_PWM_LLD_H

#if HAL_USE_PWM || defined(__DOXYGEN__)

#if !defined(CH32F20x_PWM_USE_TIM2)
#define CH32F20x_PWM_USE_TIM2 FALSE
#endif
#if !defined(CH32F20x_PWM_USE_TIM3)
#define CH32F20x_PWM_USE_TIM3 FALSE
#endif
#if !defined(CH32F20x_PWM_USE_TIM4)
#define CH32F20x_PWM_USE_TIM4 FALSE
#endif

#define PWM_CHANNELS 4U
typedef uint8_t  pwmmode_t;
typedef uint8_t  pwmchannel_t;
typedef uint8_t  pwmchnmsk_t;
typedef uint16_t pwmcnt_t;

typedef struct {
    pwmmode_t mode;
    pwmcallback_t callback;
} PWMChannelConfig;

typedef struct {
    uint32_t frequency;
    pwmcnt_t period;
    pwmcallback_t callback;
    PWMChannelConfig channels[PWM_CHANNELS];
} PWMConfig;

struct PWMDriver {
    pwmstate_t state;
    const PWMConfig *config;
    pwmcnt_t period;
    pwmchnmsk_t enabled;
    pwmchannel_t channels;
    TIM_TypeDef *tim;
};

#if CH32F20x_PWM_USE_TIM2 || defined(__DOXYGEN__)
extern PWMDriver PWMD2;
#endif
#if CH32F20x_PWM_USE_TIM3 || defined(__DOXYGEN__)
extern PWMDriver PWMD3;
#endif
#if CH32F20x_PWM_USE_TIM4 || defined(__DOXYGEN__)
extern PWMDriver PWMD4;
#endif

#ifdef __cplusplus
extern "C" {
#endif
void pwm_lld_init(void);
void pwm_lld_start(PWMDriver *pwmp);
void pwm_lld_stop(PWMDriver *pwmp);
void pwm_lld_change_period(PWMDriver *pwmp, pwmcnt_t period);
void pwm_lld_enable_channel(PWMDriver *pwmp, pwmchannel_t channel, pwmcnt_t width);
void pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t channel);
void pwm_lld_enable_periodic_notification(PWMDriver *pwmp);
void pwm_lld_disable_periodic_notification(PWMDriver *pwmp);
void pwm_lld_enable_channel_notification(PWMDriver *pwmp, pwmchannel_t channel);
void pwm_lld_disable_channel_notification(PWMDriver *pwmp, pwmchannel_t channel);
#ifdef __cplusplus
}
#endif
#endif
#endif
