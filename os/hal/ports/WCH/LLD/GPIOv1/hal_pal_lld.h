/*
 * Copyright 2026 TuStation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    GPIOv1/hal_pal_lld.h
 * @brief   CH32F20x PAL low level driver header.
 *
 * @addtogroup PAL
 * @{
 */

#ifndef HAL_PAL_LLD_H
#define HAL_PAL_LLD_H

#include "ch32f20x.h"

/*===========================================================================*/
/* Unsupported modes and specific modes                                      */
/*===========================================================================*/

/**
 * @name    CH32-specific I/O mode flags
 * @{
 */
/**
 * @brief   CH32 alternate function push-pull mode.
 */
#define PAL_MODE_CH32_ALTERNATE_PUSHPULL  16

/**
 * @brief   CH32 alternate function open-drain mode.
 */
#define PAL_MODE_CH32_ALTERNATE_OPENDRAIN 17
/** @} */

/*===========================================================================*/
/* I/O Ports Types and constants.                                            */
/*===========================================================================*/

/**
 * @brief   Port Identifier.
 */
typedef GPIO_TypeDef *ioportid_t;

/**
 * @brief   Digital I/O port sized unsigned type.
 */
typedef uint32_t ioportmask_t;

/**
 * @brief   Type of a pad identifier.
 */
typedef uint32_t iopadid_t;

/**
 * @brief   Digital I/O modes.
 */
typedef uint32_t iomode_t;

/*===========================================================================*/
/* I/O Ports Identifiers.                                                    */
/*===========================================================================*/

/**
 * @brief   GPIO port A identifier.
 */
#define IOPORT1 GPIOA

/**
 * @brief   GPIO port B identifier.
 */
#define IOPORT2 GPIOB

/**
 * @brief   GPIO port C identifier.
 */
#define IOPORT3 GPIOC

/**
 * @brief   GPIO port D identifier.
 */
#define IOPORT4 GPIOD

/**
 * @brief   Type of an I/O line.
 */
typedef uint32_t ioline_t;

/**
 * @brief   Width, in bits, of an I/O port.
 */
#define PAL_IOPORTS_WIDTH 16

/**
 * @brief   Whole port mask.
 */
#define PAL_WHOLE_PORT ((ioportmask_t)0xFFFFU)

/**
 * @brief   Number of low address bits used to encode a pad in a line.
 */
#define CH32_PAL_PAD_BITS 5U

/**
 * @brief   Forms a line identifier.
 * @details The pad number is ORed into the low five bits of the GPIO address.
 */
#define PAL_LINE(port, pad) ((ioline_t)((uint32_t)(port)) | ((uint32_t)(pad)))

/**
 * @brief   Decodes a port identifier from a line identifier.
 */
#define PAL_PORT(line)  ((ioportid_t)((line) & ~((1U << CH32_PAL_PAD_BITS) - 1U)))

/**
 * @brief   Decodes a pad identifier from a line identifier.
 */
#define PAL_PAD(line)   ((uint32_t)((line) & ((1U << CH32_PAL_PAD_BITS) - 1U)))

/**
 * @brief   Value identifying an invalid line.
 */
#define PAL_NOLINE      0U

/**
 * @brief   CH32 GPIO static initializer.
 * @note    The current implementation has no board-provided initial pin state.
 */
typedef struct {
    uint32_t dummy;
} PALConfig;

/*===========================================================================*/
/* Implementation, some of the following macros could be implemented as      */
/* functions, if so please put them in hal_pal_lld.c.                        */
/*===========================================================================*/

/**
 * @brief   GPIO ports subsystem initialization.
 *
 * @notapi
 */
#define pal_lld_init(config)           _pal_lld_init(config)

/**
 * @brief   Reads an I/O port.
 * @details Reads the GPIO input data register.
 *
 * @param[in] port      port identifier
 * @return              The port bits.
 *
 * @notapi
 */
#define pal_lld_readport(port)         ((port)->INDR)

/**
 * @brief   Reads the output latch.
 *
 * @param[in] port      port identifier
 * @return              The latched logical states.
 *
 * @notapi
 */
#define pal_lld_readlatch(port)        ((port)->OUTDR)

/**
 * @brief   Writes an I/O port.
 * @note    Writing a pad configured as pull-up or pull-down also selects the
 *          resistor direction.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be written
 *
 * @notapi
 */
#define pal_lld_writeport(port, bits)  ((port)->OUTDR = (bits))

/**
 * @brief   Sets a bit mask on an I/O port.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be set
 *
 * @notapi
 */
#define pal_lld_setport(port, bits)    ((port)->BSHR = (bits))

/**
 * @brief   Clears a bit mask on an I/O port.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be cleared
 *
 * @notapi
 */
#define pal_lld_clearport(port, bits)  ((port)->BCR = (bits))

/**
 * @brief   Toggles a bit mask on an I/O port.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be toggled
 *
 * @notapi
 */
#define pal_lld_toggleport(port, bits) ((port)->OUTDR ^= (bits))

/**
 * @brief   Writes a group of bits atomically.
 * @details Uses the set/reset halves of the BSHR register.
 *
 * @param[in] port      port identifier
 * @param[in] mask      group mask
 * @param[in] offset    group bit offset within the port
 * @param[in] bits      bits to be written
 *
 * @notapi
 */
#define pal_lld_writegroup(port, mask, offset, bits)                        \
    do {                                                                    \
        uint32_t w = ((~(uint32_t)(bits) & (uint32_t)(mask)) <<             \
                      (16U + (offset))) |                                   \
                     (((uint32_t)(bits) & (uint32_t)(mask)) << (offset));   \
        (port)->BSHR = w;                                                   \
    } while (false)

/**
 * @brief   Reads a pad logic state.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number
 * @return              The pad logical state.
 *
 * @notapi
 */
#define pal_lld_readpad(port, pad)     (((port)->INDR >> (pad)) & 1U)

/**
 * @brief   Writes a pad logic state.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number
 * @param[in] bit       logical state
 *
 * @notapi
 */
#define pal_lld_writepad(port, pad, bit) pal_lld_writegroup(port, 1U, pad, bit)

/**
 * @brief   Sets a pad to logic high.
 */
#define pal_lld_setpad(port, pad)      ((port)->BSHR = (1UL << (pad)))

/**
 * @brief   Sets a pad to logic low.
 */
#define pal_lld_clearpad(port, pad)    ((port)->BCR = (1UL << (pad)))

/**
 * @brief   Toggles a pad logic state.
 */
#define pal_lld_togglepad(port, pad)   ((port)->OUTDR ^= (1UL << (pad)))

/**
 * @brief   Configures a group of pads with the same mode.
 *
 * @param[in] port      port identifier
 * @param[in] mask      group mask
 * @param[in] offset    group bit offset within the port
 * @param[in] mode      group mode
 *
 * @notapi
 */
#define pal_lld_setgroupmode(port, mask, offset, mode) \
    _pal_lld_setgroupmode((port), (mask) << (offset), (mode))

#if !defined(__DOXYGEN__)
extern const PALConfig pal_default_config;
#endif

#ifdef __cplusplus
extern "C" {
#endif
void _pal_lld_init(const PALConfig *config);
void _pal_lld_setgroupmode(ioportid_t port, ioportmask_t mask, iomode_t mode);
#ifdef __cplusplus
}
#endif

#endif /* HAL_PAL_LLD_H */

/** @} */
