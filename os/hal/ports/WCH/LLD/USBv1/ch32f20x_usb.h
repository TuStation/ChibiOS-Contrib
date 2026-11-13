/*
 * @file    USBv1/ch32f20x_usb.h
 * @brief   CH32F203 (CB/RB/RC/VC = CH32F20x_D8 class) USB FS device registers.
 *
 * @details The CB part uses the STM32F1-compatible USB FS device block:
 *
 *            base    0x40005C00  (endpoint registers at +0x00)
 *            PMA     0x40006000
 *            CNTR    +0x40   ISTR +0x44   FNR +0x48
 *            DADDR   +0x4C   BTABLE +0x50
 *
 *          Register layout follows the WCH CH32F20x USB Device library
 *          (usb_regs.h: RegBase 0x40005C00, PMAAddr 0x40006000,
 *          BTABLE RegBase+0x50).
 */

#ifndef CH32F20X_USB_H
#define CH32F20X_USB_H

#include "ch32f20x.h"

#define USB_ENDPOINTS_NUMBER    7   /* EP1..EP7, EP0 always present */

/* The CMSIS USBD_BASE macros use AHBPERIPH_BASE and point 0x20000 too
 * high for this USB device block, so use the APB1 register map. */
#define CH32_USB_BASE           (APB1PERIPH_BASE + 0x5C00U)
#define CH32_USBRAM_BASE        (APB1PERIPH_BASE + 0x6000U)

/*
 * The USB PMA is physically 32-bit spaced: only the low halfword of each
 * slot is significant.  Addresses stored in the buffer table are logical
 * byte offsets, therefore every PMA access is widened to a 32-bit slot.
 */
typedef uint32_t ch32_usb_pma_t;

/* Endpoint registers: 8 x 32-bit slots, only the low 16 bits are used. */
typedef struct {
    volatile uint32_t EPR[USB_ENDPOINTS_NUMBER + 1];
    volatile uint32_t _reserved[8];      /* 0x20..0x3F */
    volatile uint32_t CNTR;              /* 0x40, low 16 bits used */
    volatile uint32_t ISTR;              /* 0x44, low 16 bits used */
    volatile uint32_t FNR;               /* 0x48, low 16 bits used */
    volatile uint32_t DADDR;             /* 0x4C, low 16 bits used */
    volatile uint32_t BTABLE;            /* 0x50, low 16 bits used */
} ch32_usb_t;

/* Buffer descriptor table entry (4 half-words per endpoint) */
typedef struct {
    volatile ch32_usb_pma_t TXADDR0;
    volatile ch32_usb_pma_t TXCOUNT0;
    volatile ch32_usb_pma_t RXADDR0;
    volatile ch32_usb_pma_t RXCOUNT0;
} ch32_usb_descriptor_t;

#define RXCOUNT1                TXCOUNT0
#define TXCOUNT1                RXCOUNT0
#define RXADDR1                 TXADDR0
#define TXADDR1                 RXADDR0

#define CH32_USB                ((ch32_usb_t *)CH32_USB_BASE)
#define CH32_USBRAM             ((ch32_usb_pma_t *)CH32_USBRAM_BASE)

/* EPR bits (STM32-compatible) */
#define EPR_EA_MASK             0x000F
#define EPR_STAT_TX_MASK        0x0030
#define EPR_STAT_TX_DIS         0x0000
#define EPR_STAT_TX_STALL       0x0010
#define EPR_STAT_TX_NAK         0x0020
#define EPR_STAT_TX_VALID       0x0030
#define EPR_DTOG_TX             0x0040
#define EPR_CTR_TX              0x0080
#define EPR_EP_KIND             0x0100
#define EPR_EP_TYPE_MASK        0x0600
#define EPR_EP_TYPE_BULK        0x0000
#define EPR_EP_TYPE_CONTROL     0x0200
#define EPR_EP_TYPE_ISO         0x0400
#define EPR_EP_TYPE_INTERRUPT   0x0600
#define EPR_SETUP               0x0800
#define EPR_STAT_RX_MASK        0x3000
#define EPR_STAT_RX_DIS         0x0000
#define EPR_STAT_RX_STALL       0x1000
#define EPR_STAT_RX_NAK         0x2000
#define EPR_STAT_RX_VALID       0x3000
#define EPR_DTOG_RX             0x4000
#define EPR_CTR_RX              0x8000

/* CNTR bits */
#define CNTR_FRES               0x0001
#define CNTR_PDWN               0x0002
#define CNTR_LP_MODE            0x0004
#define CNTR_FSUSP              0x0008
#define CNTR_RESUME             0x0010
#define CNTR_L2RES              0x0010
#define CNTR_ESOFM              0x0100
#define CNTR_SOFM               0x0200
#define CNTR_RESETM             0x0400
#define CNTR_SUSPM              0x0800
#define CNTR_WKUPM              0x1000
#define CNTR_ERRM               0x2000
#define CNTR_PMAOVRM            0x4000
#define CNTR_CTRM               0x8000

/* ISTR bits */
#define ISTR_EP_ID_MASK         0x000F
#define ISTR_DIR                0x0010
#define ISTR_SOF                0x0200
#define ISTR_RESET              0x0400
#define ISTR_SUSP               0x0800
#define ISTR_WKUP               0x1000
#define ISTR_ERR                0x2000
#define ISTR_L1REQ              0x4000
#define ISTR_CTR                0x8000

/* FNR / DADDR */
#define FNR_FN_MASK             0x07FF
#define FNR_RXDM                0x4000
#define FNR_RXDP                0x8000
#define FNR_LSOF_MASK           0xC000
#define DADDR_EF                0x80
#define DADDR_ADD_MASK          0x7F

/* Toggle-preserving write helpers (EPRx STAT/DTOG bits are XOR-to-write) */
#define EPR_TOGGLE_MASK         (EPR_STAT_TX_MASK | EPR_DTOG_TX | \
                                 EPR_STAT_RX_MASK | EPR_DTOG_RX | \
                                 EPR_SETUP)

#define EPR_CTR_MASK            (EPR_CTR_TX | EPR_CTR_RX)

/*
 * STAT and DTOG fields are XOR-to-write.  The CH32F20x vendor library also
 * requires both CTR bits to be written as one when they are not being
 * acknowledged, otherwise an unrelated transfer completion is lost.
 */
#define EPR_SET_STAT_RX(ep, stat)                                         \
    do {                                                                   \
        uint32_t epr_ = CH32_USB->EPR[(ep)];                              \
        CH32_USB->EPR[(ep)] =                                             \
            (epr_ & (EPR_EA_MASK | EPR_EP_TYPE_MASK | EPR_EP_KIND)) |    \
            ((epr_ & EPR_STAT_RX_MASK) ^ (uint32_t)(stat)) |             \
            EPR_CTR_MASK;                                                 \
    } while (false)

#define EPR_SET_STAT_TX(ep, stat)                                         \
    do {                                                                   \
        uint32_t epr_ = CH32_USB->EPR[(ep)];                              \
        CH32_USB->EPR[(ep)] =                                             \
            (epr_ & (EPR_EA_MASK | EPR_EP_TYPE_MASK | EPR_EP_KIND)) |    \
            ((epr_ & EPR_STAT_TX_MASK) ^ (uint32_t)(stat)) |             \
            EPR_CTR_MASK;                                                 \
    } while (false)

#define EPR_CLEAR_CTR_RX(ep)                                              \
    do {                                                                   \
        uint32_t epr_ = CH32_USB->EPR[(ep)];                              \
        CH32_USB->EPR[(ep)] =                                             \
            (epr_ & (EPR_EA_MASK | EPR_EP_TYPE_MASK | EPR_EP_KIND)) |    \
            EPR_CTR_TX;                                                   \
    } while (false)

#define EPR_CLEAR_CTR_TX(ep)                                              \
    do {                                                                   \
        uint32_t epr_ = CH32_USB->EPR[(ep)];                              \
        CH32_USB->EPR[(ep)] =                                             \
            (epr_ & (EPR_EA_MASK | EPR_EP_TYPE_MASK | EPR_EP_KIND)) |    \
            EPR_CTR_RX;                                                   \
    } while (false)

/* Buffer-table and PMA-address conversion helpers. */
#define USB_GET_DESCRIPTOR(ep)                                            \
    ((ch32_usb_descriptor_t *)(CH32_USBRAM_BASE +                         \
                               (uint32_t)CH32_USB->BTABLE *               \
                                   (sizeof(ch32_usb_pma_t) / 2U) +         \
                               (uint32_t)(ep) *                           \
                               sizeof(ch32_usb_descriptor_t)))

#define USB_ADDR2PTR(addr)                                                \
    ((ch32_usb_pma_t *)((uint32_t)(addr) *                                \
                        (sizeof(ch32_usb_pma_t) / 2U) +                   \
                        CH32_USBRAM_BASE))

#endif /* CH32F20X_USB_H */
