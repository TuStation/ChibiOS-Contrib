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

/**
 * @file    USBv1/hal_usb_lld.c
 * @brief   CH32F203CB (CH32F20x_D8) USB FS device low level driver.
 *
 * @details Adapted from the ChibiOS STM32 USBv1 low-level driver for the WCH
 *          CH32F203 USB FS controller.
 *
 *          The CB/RB/RC/VC parts expose the STM32F1-compatible USB FS block
 *          (not the WCH USBFS SIE used on K8/C6/C8):
 *
 *            USB registers  0x40005C00  (EPR0-7 at +0x00)
 *            PMA            0x40006000
 *            CNTR +0x40  ISTR +0x44  FNR +0x48  DADDR +0x4C  BTABLE +0x50
 *            IRQ            USB_LP_CAN1_RX0_IRQn = 20 -> ChibiOS Vector90
 *            pull-up        EXTEN_CTR (0x40023800) bit 1
 *
 *          Hardware register layout and initialization sequence follow the
 *          WCH CH32F20x reference materials and USB Device library.
 *
 * @addtogroup USB
 * @{
 */

#include "hal.h"
#include <string.h>

#if HAL_USE_USB || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

USBDriver USBD1;

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Buffer descriptor table: eight entries of eight logical PMA bytes. */
#define CH32_BTABLE_ADDR         0U
#define CH32_BTABLE_SIZE         64U

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

static union {
    USBInEndpointState  in;
    USBOutEndpointState out;
} ep0_state;

static uint8_t ep0setup_buffer[8];

static const USBEndpointConfig ep0config = {
    .ep_mode      = USB_EP_MODE_TYPE_CTRL,
    .setup_cb     = _usb_ep0setup,
    .in_cb        = _usb_ep0in,
    .out_cb       = _usb_ep0out,
    .in_maxsize   = USB_EP0_PACKET_SIZE,
    .out_maxsize  = USB_EP0_PACKET_SIZE,
    .in_state     = &ep0_state.in,
    .out_state    = &ep0_state.out,
    .ep_buffers   = 1U,
    .setup_buf    = ep0setup_buffer,
};

/*===========================================================================*/
/* Driver local helpers.                                                     */
/*===========================================================================*/

static void usb_pm_reset(USBDriver *usbp) {
    usbp->pmnext = CH32_BTABLE_ADDR + CH32_BTABLE_SIZE;
}

static uint32_t usb_pm_alloc(USBDriver *usbp, size_t size) {
    uint32_t next = usbp->pmnext;
    usbp->pmnext += ((uint32_t)size + 1U) & ~1U;
    osalDbgAssert(usbp->pmnext <= CH32_USB_PMA_SIZE, "PMA overflow");
    return next;
}

static uint32_t usb_rx_count(size_t size) {
    if (size > 62U) {
        return 0x8000U | ((((uint32_t)size + 31U) / 32U - 1U) << 10);
    }
    return (((uint32_t)size + 1U) / 2U) << 10;
}

static size_t usb_packet_read_to_buffer(usbep_t ep, uint8_t *buf, size_t maxsize) {
    ch32_usb_descriptor_t *descriptor = USB_GET_DESCRIPTOR(ep);
    ch32_usb_pma_t *pma = USB_ADDR2PTR(descriptor->RXADDR0);
    size_t n = (size_t)descriptor->RXCOUNT0 & RXCOUNT_COUNT_MASK;
    if (n > maxsize) {
        n = maxsize;
    }
    size_t remaining = n;

    while (remaining >= 2U) {
        uint32_t word = *pma++;
        *buf++ = (uint8_t)word;
        *buf++ = (uint8_t)(word >> 8);
        remaining -= 2U;
    }
    if (remaining != 0U) {
        *buf = (uint8_t)*pma;
    }
    return n;
}

static void usb_packet_write_from_buffer(usbep_t ep, const uint8_t *buf, size_t n) {
    ch32_usb_descriptor_t *descriptor = USB_GET_DESCRIPTOR(ep);
    ch32_usb_pma_t *pma = USB_ADDR2PTR(descriptor->TXADDR0);
    size_t remaining = n;

    descriptor->TXCOUNT0 = (ch32_usb_pma_t)n;
    while (remaining >= 2U) {
        *pma++ = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8);
        buf += 2;
        remaining -= 2U;
    }
    if (remaining != 0U) {
        *pma = *buf;
    }
}

static void usb_serve_endpoints(USBDriver *usbp, uint32_t istr) {
    usbep_t ep = (usbep_t)(istr & ISTR_EP_ID_MASK);
    uint32_t epr = CH32_USB->EPR[ep];
    const USBEndpointConfig *epcp = usbp->epc[ep];

    if (epcp == NULL) {
        if ((istr & ISTR_DIR) == 0U) {
            EPR_CLEAR_CTR_TX(ep);
        } else {
            EPR_CLEAR_CTR_RX(ep);
        }
        return;
    }

    if ((istr & ISTR_DIR) == 0U) {
        USBInEndpointState *isp = epcp->in_state;
        EPR_CLEAR_CTR_TX(ep);
        if (isp == NULL) {
            return;
        }

        isp->txcnt += isp->txlast;
        size_t n = isp->txsize - isp->txcnt;
        if (n > 0U) {
            if (n > epcp->in_maxsize) {
                n = epcp->in_maxsize;
            }
            isp->txbuf += isp->txlast;
            isp->txlast = n;
            usb_packet_write_from_buffer(ep, isp->txbuf, n);
            EPR_SET_STAT_TX(ep, EPR_STAT_TX_VALID);
        } else {
            _usb_isr_invoke_in_cb(usbp, ep);
        }
    } else {
        EPR_CLEAR_CTR_RX(ep);
        if ((epr & EPR_SETUP) != 0U) {
            usb_packet_read_to_buffer(ep, usbp->setup, sizeof(usbp->setup));
            _usb_isr_invoke_setup_cb(usbp, ep);
            return;
        }

        USBOutEndpointState *osp = epcp->out_state;
        if (osp == NULL) {
            return;
        }
        size_t n = usb_packet_read_to_buffer(ep, osp->rxbuf, osp->rxsize);
        osp->rxbuf += n;
        osp->rxcnt += n;
        osp->rxsize -= n;
        if (osp->rxpkts > 0U) {
            osp->rxpkts--;
        }

        if ((n < epcp->out_maxsize) || (osp->rxpkts == 0U)) {
            _usb_isr_invoke_out_cb(usbp, ep);
        } else {
            EPR_SET_STAT_RX(ep, EPR_STAT_RX_VALID);
        }
    }
}

/*===========================================================================*/
/* Driver interrupt handler.                                                 */
/*===========================================================================*/

OSAL_IRQ_HANDLER(Vector90) {
    OSAL_IRQ_PROLOGUE();

    uint32_t istr = CH32_USB->ISTR;
    CH32_USB->ISTR = ~istr;

    if ((istr & ISTR_RESET) != 0U) {
        _usb_reset(&USBD1);
    }
    if ((istr & ISTR_SUSP) != 0U) {
        CH32_USB->CNTR |= CNTR_FSUSP;
        _usb_suspend(&USBD1);
    }
    if ((istr & ISTR_WKUP) != 0U) {
        CH32_USB->CNTR &= ~CNTR_FSUSP;
        _usb_wakeup(&USBD1);
    }
    if (((istr & ISTR_SOF) != 0U) && (USBD1.config != NULL)) {
        _usb_isr_invoke_sof_cb(&USBD1);
    }

    while (((istr = CH32_USB->ISTR) & ISTR_CTR) != 0U) {
        usb_serve_endpoints(&USBD1, istr);
    }

    OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void usb_lld_init(void) {
    usbObjectInit(&USBD1);
}

void usb_lld_start(USBDriver *usbp) {
    if (usbp->state == USB_STOP) {
        /* Re-enable and reset USBD on every USB restart. */
        RCC->APB1PCENR |= RCC_USBEN;
        RCC->APB1PRSTR |= RCC_USBRST;
        RCC->APB1PRSTR &= ~RCC_USBRST;
        CH32_USB->CNTR = CNTR_FRES;
        CH32_USB->CNTR = 0U;
        CH32_USB->ISTR = 0U;
        CH32_USB->BTABLE = CH32_BTABLE_ADDR;
        CH32_USB->DADDR = 0U;
        nvicEnableVector(USB_LP_CAN1_RX0_IRQn, 5);
        usb_lld_reset(usbp);
    }
}

void usb_lld_stop(USBDriver *usbp) {
    if (usbp->state != USB_STOP) {
        nvicDisableVector(USB_LP_CAN1_RX0_IRQn);
        CH32_USB->CNTR = CNTR_FRES | CNTR_PDWN;
    }
}

void usb_lld_reset(USBDriver *usbp) {
    uint32_t cntr = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM | CNTR_ERRM |
                    CNTR_CTRM;

    CH32_USB->BTABLE = CH32_BTABLE_ADDR;
    CH32_USB->ISTR = 0U;
    CH32_USB->DADDR = DADDR_EF;
    if ((usbp->config != NULL) && (usbp->config->sof_cb != NULL)) {
        cntr |= CNTR_SOFM;
    }
    CH32_USB->CNTR = cntr;
    for (usbep_t ep = 0U; ep <= USB_MAX_ENDPOINTS; ep++) {
        /* Clear DTOG and STAT XOR bits before disabling the endpoint. */
        CH32_USB->EPR[ep] = CH32_USB->EPR[ep];
        CH32_USB->EPR[ep] = 0U;
    }

    usb_pm_reset(usbp);
    usbp->epc[0] = &ep0config;
    usbp->address = 0U;
    usb_lld_init_endpoint(usbp, 0U);
}

void usb_lld_set_address(USBDriver *usbp) {
    CH32_USB->DADDR = DADDR_EF | (usbp->address & DADDR_ADD_MASK);
}

void usb_lld_init_endpoint(USBDriver *usbp, usbep_t ep) {
    const USBEndpointConfig *epcp = usbp->epc[ep];
    uint32_t epr;

    if (epcp == NULL) {
        return;
    }

    switch (epcp->ep_mode & 0x03U) {
    case USB_EP_MODE_TYPE_CTRL:
        epr = EPR_EP_TYPE_CONTROL;
        break;
    case USB_EP_MODE_TYPE_ISOC:
        epr = EPR_EP_TYPE_ISO;
        break;
    case USB_EP_MODE_TYPE_BULK:
        epr = EPR_EP_TYPE_BULK;
        break;
    default:
        epr = EPR_EP_TYPE_INTERRUPT;
        break;
    }

    ch32_usb_descriptor_t *descriptor = USB_GET_DESCRIPTOR(ep);
    if (epcp->in_state != NULL) {
        descriptor->TXADDR0 = usb_pm_alloc(usbp, epcp->in_maxsize);
        descriptor->TXCOUNT0 = 0U;
        epr |= EPR_STAT_TX_NAK;
    }
    if (epcp->out_state != NULL) {
        descriptor->RXADDR0 = usb_pm_alloc(usbp, epcp->out_maxsize);
        descriptor->RXCOUNT0 = usb_rx_count(epcp->out_maxsize);
        epr |= EPR_STAT_RX_NAK;
    }

    /* EPR STAT/DTOG fields are write-one-to-toggle. Clear them before
       setting endpoint type, address, and initial NAK states. */
    CH32_USB->EPR[ep] = CH32_USB->EPR[ep];
    CH32_USB->EPR[ep] = epr | ep;
}

void usb_lld_disable_endpoints(USBDriver *usbp) {
    for (usbep_t ep = 1U; ep <= USB_MAX_ENDPOINTS; ep++) {
        CH32_USB->EPR[ep] = CH32_USB->EPR[ep];
        CH32_USB->EPR[ep] = 0U;
    }

    /* EP0 remains enabled and keeps its TX and RX buffers. */
    usbp->pmnext = CH32_BTABLE_ADDR + CH32_BTABLE_SIZE +
                   2U * USB_EP0_PACKET_SIZE;
}

usbepstatus_t usb_lld_get_status_in(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    switch (CH32_USB->EPR[ep] & EPR_STAT_TX_MASK) {
    case EPR_STAT_TX_DIS:
        return EP_STATUS_DISABLED;
    case EPR_STAT_TX_STALL:
        return EP_STATUS_STALLED;
    default:
        return EP_STATUS_ACTIVE;
    }
}

usbepstatus_t usb_lld_get_status_out(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    switch (CH32_USB->EPR[ep] & EPR_STAT_RX_MASK) {
    case EPR_STAT_RX_DIS:
        return EP_STATUS_DISABLED;
    case EPR_STAT_RX_STALL:
        return EP_STATUS_STALLED;
    default:
        return EP_STATUS_ACTIVE;
    }
}

void usb_lld_read_setup(USBDriver *usbp, usbep_t ep, uint8_t *buf) {
    (void)ep;
    memcpy(buf, usbp->setup, sizeof(usbp->setup));
}

void usb_lld_end_setup(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    EPR_SET_STAT_RX(ep, EPR_STAT_RX_VALID);
}

void usb_lld_start_in(USBDriver *usbp, usbep_t ep) {
    const USBEndpointConfig *epcp = usbp->epc[ep];
    USBInEndpointState *isp = epcp->in_state;
    size_t n = isp->txsize - isp->txcnt;

    if (n > epcp->in_maxsize) {
        n = epcp->in_maxsize;
    }
    isp->txlast = n;
    usb_packet_write_from_buffer(ep, isp->txbuf + isp->txcnt, n);
    EPR_SET_STAT_TX(ep, EPR_STAT_TX_VALID);
}

void usb_lld_start_out(USBDriver *usbp, usbep_t ep) {
    const USBEndpointConfig *epcp = usbp->epc[ep];
    USBOutEndpointState *osp = epcp->out_state;

    osp->rxpkts = (uint16_t)((osp->rxsize + epcp->out_maxsize - 1U) /
                             epcp->out_maxsize);
    if (osp->rxpkts == 0U) {
        osp->rxpkts = 1U;
    }
    USB_GET_DESCRIPTOR(ep)->RXCOUNT0 = usb_rx_count(epcp->out_maxsize);
    EPR_SET_STAT_RX(ep, EPR_STAT_RX_VALID);
}

void usb_lld_prepare_receive(USBDriver *usbp, usbep_t ep, uint8_t *buf,
                             size_t n) {
    USBOutEndpointState *osp = usbp->epc[ep]->out_state;
    osp->rxbuf = buf;
    osp->rxsize = n;
    osp->rxcnt = 0U;
    usb_lld_start_out(usbp, ep);
}

void usb_lld_prepare_send(USBDriver *usbp, usbep_t ep, const uint8_t *buf,
                          size_t n) {
    USBInEndpointState *isp = usbp->epc[ep]->in_state;
    isp->txbuf = buf;
    isp->txsize = n;
    isp->txcnt = 0U;
    usb_lld_start_in(usbp, ep);
}

void usb_lld_stall_in(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    EPR_SET_STAT_TX(ep, EPR_STAT_TX_STALL);
}

void usb_lld_stall_out(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    EPR_SET_STAT_RX(ep, EPR_STAT_RX_STALL);
}

void usb_lld_clear_in(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    EPR_SET_STAT_TX(ep, EPR_STAT_TX_NAK);
}

void usb_lld_clear_out(USBDriver *usbp, usbep_t ep) {
    (void)usbp;
    EPR_SET_STAT_RX(ep, EPR_STAT_RX_VALID);
}

#endif /* HAL_USE_USB */

/** @} */
