/*
 * Copyright (C) 2025 Steward <steward.fu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef __CHAR_F1C100S_UART_H__
#define __CHAR_F1C100S_UART_H__

#include "hw/sysbus.h"
#include "chardev/char-fe.h"
#include "qom/object.h"
#include "qemu/fifo8.h"

#define TYPE_AW_UART "f1c100s-uart"
OBJECT_DECLARE_SIMPLE_TYPE(AwUartState, AW_UART)

#define AW_UART_FIFO_SIZE 64

struct AwUartState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq irq;
    CharBackend chr;

    /* 16550-compatible registers */
    uint8_t rbr;        /* Receive Buffer Register (read-only) */
    uint8_t thr;        /* Transmit Holding Register (write-only) */
    uint8_t dll;        /* Divisor Latch Low (when DLAB=1) */
    uint8_t dlh;        /* Divisor Latch High (when DLAB=1) */
    uint8_t ier;        /* Interrupt Enable Register */
    uint8_t iir;        /* Interrupt Identification Register (read-only) */
    uint8_t fcr;        /* FIFO Control Register (write-only) */
    uint8_t lcr;        /* Line Control Register */
    uint8_t mcr;        /* Modem Control Register */
    uint8_t lsr;        /* Line Status Register */
    uint8_t msr;        /* Modem Status Register */
    uint8_t scr;        /* Scratch Register */

    /* Allwinner-specific registers */
    uint32_t usr;       /* UART Status Register */
    uint32_t tfl;       /* Transmit FIFO Level */
    uint32_t rfl;       /* Receive FIFO Level */
    uint32_t halt;      /* HALT TX register */

    Fifo8 rx_fifo;
    Fifo8 tx_fifo;

    uint8_t rx_fifo_trigger;
    uint8_t tx_fifo_trigger;
    bool fifo_enabled;
    bool thr_ipending;
};

#endif
