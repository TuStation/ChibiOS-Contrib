/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
 * @file    USBv1/hal_usb_lld.h
 * @brief   CH32F203 USB device driver header.
 *
 * Adapted from the ChibiOS STM32 USBv1 low-level driver for the WCH CH32F203
 * USB FS controller.
 * Key differences: 16-bit regs, swapped control/EP offsets, EXTEN_CTR pull-up.
 */

#ifndef HAL_USB_LLD_H
#define HAL_USB_LLD_H

#if HAL_USE_USB || defined(__DOXYGEN__)

#include "ch32f20x_usb.h"

#define USB_MAX_ENDPOINTS        USB_ENDPOINTS_NUMBER
#define USB_EP0_STATUS_STAGE     USB_EP0_STATUS_STAGE_SW
#define USB_SET_ADDRESS_MODE     USB_LATE_SET_ADDRESS
#define USB_SET_ADDRESS_ACK_HANDLING USB_SET_ADDRESS_ACK_SW

#define USB_EP0_PACKET_SIZE      64
#define USB_EP0_MAX_SIZE         64
#define USB_EP_MAX_SIZE          64

/* PMA size: 512 bytes */
#define CH32_USB_PMA_SIZE        512

/* RX counter count mask */
#define RXCOUNT_COUNT_MASK       0x03FF

/*
 * CH32F203CBT6 uses the vendor USBD port sequence.  The D+ pull-up alone is
 * not sufficient after a software disconnect: the transceiver must leave
 * power-down and PA11/PA12 must be returned to their USB input mode first.
 */
#if !defined(usb_lld_connect_bus)
#define usb_lld_connect_bus(usbp)                                             \
  do {                                                                        \
    (void)(usbp);                                                             \
    CH32_USB->CNTR &= ~CNTR_PDWN;                                             \
    GPIOA->OUTDR &= ~((1U << 11) | (1U << 12));                              \
    GPIOA->CFGHR = (GPIOA->CFGHR & ~((0xFU << 12) | (0xFU << 16))) |         \
                   (0x8U << 12) | (0x8U << 16);                             \
    EXTEN->EXTEN_CTR |= EXTEN_USBD_PU_EN;                                     \
  } while (false)
#endif

#if !defined(usb_lld_disconnect_bus)
#define usb_lld_disconnect_bus(usbp)                                          \
  do {                                                                        \
    (void)(usbp);                                                             \
    EXTEN->EXTEN_CTR &= ~EXTEN_USBD_PU_EN;                                    \
    CH32_USB->CNTR |= CNTR_PDWN;                                              \
    GPIOA->OUTDR &= ~((1U << 11) | (1U << 12));                              \
    GPIOA->CFGHR = (GPIOA->CFGHR & ~((0xFU << 12) | (0xFU << 16))) |         \
                   (0x3U << 12) | (0x3U << 16);                             \
  } while (false)
#endif

#define usb_lld_get_frame_number(usbp) (CH32_USB->FNR & FNR_FN_MASK)

#define usb_lld_get_transaction_size(usbp, ep)                                \
  ((usbp)->epc[ep]->out_state->rxcnt)

#define usb_lld_wakeup_host(usbp)                                             \
  do {                                                                        \
    CH32_USB->CNTR |= CNTR_RESUME;                                            \
    osalThreadSleepMilliseconds(2);                                           \
    CH32_USB->CNTR &= ~CNTR_RESUME;                                           \
  } while (false)

/* IN endpoint state */
typedef struct {
    size_t          txsize;
    size_t          txcnt;
    const uint8_t  *txbuf;
#if (USB_USE_WAIT == TRUE) || defined(__DOXYGEN__)
    thread_reference_t thread;
#endif
    size_t          txlast;
} USBInEndpointState;

/* OUT endpoint state */
typedef struct {
    size_t          rxsize;
    size_t          rxcnt;
    uint8_t        *rxbuf;
#if (USB_USE_WAIT == TRUE) || defined(__DOXYGEN__)
    thread_reference_t thread;
#endif
    uint16_t        rxpkts;
} USBOutEndpointState;

/* Endpoint config */
typedef struct {
    uint32_t           ep_mode;
    usbepcallback_t    setup_cb;
    usbepcallback_t    in_cb;
    usbepcallback_t    out_cb;
    uint16_t           in_maxsize;
    uint16_t           out_maxsize;
    USBInEndpointState *in_state;
    USBOutEndpointState *out_state;
    uint16_t           ep_buffers;
    uint8_t           *setup_buf;
} USBEndpointConfig;

/* USBConfig */
typedef struct {
    usbeventcb_t       event_cb;
    usbgetdescriptor_t get_descriptor_cb;
    usbreqhandler_t    requests_hook_cb;
    usbcallback_t      sof_cb;
} USBConfig;

/* Main USBDriver structure */
struct USBDriver {
    usbstate_t                state;
    const USBConfig          *config;
    uint16_t                  transmitting;
    uint16_t                  receiving;
    const USBEndpointConfig  *epc[USB_MAX_ENDPOINTS + 1];
    void                     *in_params[USB_MAX_ENDPOINTS];
    void                     *out_params[USB_MAX_ENDPOINTS];
    usbep0state_t             ep0state;
    uint8_t                  *ep0next;
    size_t                    ep0n;
    usbcallback_t             ep0endcb;
    uint8_t                   setup[8];
    uint16_t                  status;
    uint8_t                   address;
    uint8_t                   configuration;
    usbstate_t                saved_state;
#if defined(USB_DRIVER_EXT_FIELDS)
    USB_DRIVER_EXT_FIELDS
#endif
    uint32_t                  pmnext;
};

/* Function declarations */
#ifdef __cplusplus
extern "C" {
#endif
void usb_lld_init(void);
void usb_lld_start(USBDriver *usbp);
void usb_lld_stop(USBDriver *usbp);
void usb_lld_reset(USBDriver *usbp);
void usb_lld_set_address(USBDriver *usbp);
void usb_lld_init_endpoint(USBDriver *usbp, usbep_t ep);
void usb_lld_disable_endpoints(USBDriver *usbp);
usbepstatus_t usb_lld_get_status_in(USBDriver *usbp, usbep_t ep);
usbepstatus_t usb_lld_get_status_out(USBDriver *usbp, usbep_t ep);
void usb_lld_read_setup(USBDriver *usbp, usbep_t ep, uint8_t *buf);
void usb_lld_end_setup(USBDriver *usbp, usbep_t ep);
void usb_lld_start_in(USBDriver *usbp, usbep_t ep);
void usb_lld_start_out(USBDriver *usbp, usbep_t ep);
void usb_lld_prepare_receive(USBDriver *usbp, usbep_t ep, uint8_t *buf, size_t n);
void usb_lld_prepare_send(USBDriver *usbp, usbep_t ep, const uint8_t *buf, size_t n);
void usb_lld_stall_in(USBDriver *usbp, usbep_t ep);
void usb_lld_stall_out(USBDriver *usbp, usbep_t ep);
void usb_lld_clear_in(USBDriver *usbp, usbep_t ep);
void usb_lld_clear_out(USBDriver *usbp, usbep_t ep);

extern USBDriver USBD1;

#ifdef __cplusplus
}
#endif


#endif /* HAL_USE_USB */
#endif /* HAL_USB_LLD_H */
