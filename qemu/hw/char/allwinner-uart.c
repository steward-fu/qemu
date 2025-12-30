/*
 * Copyright (C) 2025 Steward <steward.fu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License
 or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful
 but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "qemu/osdep.h"
#include "hw/char/allwinner-uart.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/aw_log.h"
#include "qemu/module.h"
#include "trace.h"

/* Register offsets */
#define AW_UART_RBR     0x00    /* Receive Buffer Register (R) */
#define AW_UART_THR     0x00    /* Transmit Holding Register (W) */
#define AW_UART_DLL     0x00    /* Divisor Latch Low (when DLAB=1) */
#define AW_UART_DLH     0x04    /* Divisor Latch High (when DLAB=1) */
#define AW_UART_IER     0x04    /* Interrupt Enable Register */
#define AW_UART_IIR     0x08    /* Interrupt Identification Register (R) */
#define AW_UART_FCR     0x08    /* FIFO Control Register (W) */
#define AW_UART_LCR     0x0C    /* Line Control Register */
#define AW_UART_MCR     0x10    /* Modem Control Register */
#define AW_UART_LSR     0x14    /* Line Status Register */
#define AW_UART_MSR     0x18    /* Modem Status Register */
#define AW_UART_SCH     0x1C    /* Scratch Register */
#define AW_UART_USR     0x7C    /* UART Status Register (Allwinner) */
#define AW_UART_TFL     0x80    /* Transmit FIFO Level (Allwinner) */
#define AW_UART_RFL     0x84    /* Receive FIFO Level (Allwinner) */
#define AW_UART_HALT    0xA4    /* HALT TX (Allwinner) */

/* Line Control Register bits */
#define AW_UART_LCR_DLAB    0x80    /* Divisor Latch Access Bit */

/* Interrupt Enable Register bits */
#define AW_UART_IER_RDI     0x01    /* Received Data Interrupt */
#define AW_UART_IER_THRI    0x02    /* Transmitter Holding Register Empty */
#define AW_UART_IER_RLSI    0x04    /* Receiver Line Status Interrupt */
#define AW_UART_IER_MSI     0x08    /* Modem Status Interrupt */

/* Interrupt Identification Register bits */
#define AW_UART_IIR_NO_INT  0x01    /* No interrupt pending */
#define AW_UART_IIR_ID_MASK 0x0E    /* Interrupt ID mask */
#define AW_UART_IIR_MSI     0x00    /* Modem Status Interrupt */
#define AW_UART_IIR_THRI    0x02    /* Transmitter Holding Register Empty */
#define AW_UART_IIR_RDI     0x04    /* Received Data Available */
#define AW_UART_IIR_RLSI    0x06    /* Receiver Line Status Interrupt */
#define AW_UART_IIR_BUSY    0x07    /* Busy Detect */
#define AW_UART_IIR_TIMEOUT 0x0C    /* Character Timeout */
#define AW_UART_IIR_FIFO_EN 0xC0    /* FIFOs enabled */

/* FIFO Control Register bits */
#define AW_UART_FCR_FIFO_EN 0x01    /* FIFO Enable */
#define AW_UART_FCR_RX_RST  0x02    /* Receiver FIFO Reset */
#define AW_UART_FCR_TX_RST  0x04    /* Transmitter FIFO Reset */
#define AW_UART_FCR_TRG_1   0x00    /* Trigger level 1 byte */
#define AW_UART_FCR_TRG_4   0x40    /* Trigger level 1/4 FIFO */
#define AW_UART_FCR_TRG_8   0x80    /* Trigger level 1/2 FIFO */
#define AW_UART_FCR_TRG_14  0xC0    /* Trigger level 2 bytes less than full */

/* Line Status Register bits */
#define AW_UART_LSR_DR      0x01    /* Data Ready */
#define AW_UART_LSR_OE      0x02    /* Overrun Error */
#define AW_UART_LSR_PE      0x04    /* Parity Error */
#define AW_UART_LSR_FE      0x08    /* Framing Error */
#define AW_UART_LSR_BI      0x10    /* Break Interrupt */
#define AW_UART_LSR_THRE    0x20    /* Transmitter Holding Register Empty */
#define AW_UART_LSR_TEMT    0x40    /* Transmitter Empty */
#define AW_UART_LSR_FIFOERR 0x80    /* FIFO Error */

/* UART Status Register bits (Allwinner-specific) */
#define AW_UART_USR_BUSY    0x01    /* UART Busy */
#define AW_UART_USR_TFNF    0x02    /* Transmit FIFO Not Full */
#define AW_UART_USR_TFE     0x04    /* Transmit FIFO Empty */
#define AW_UART_USR_RFNE    0x08    /* Receive FIFO Not Empty */
#define AW_UART_USR_RFF     0x10    /* Receive FIFO Full */

static void aw_uart_update_irq(AwUartState *s)
{
    bool irq_pending = false;
    uint8_t iir = AW_UART_IIR_NO_INT;

    trace("call %s()\n", __func__);

    if ((s->ier & AW_UART_IER_RLSI) && (s->lsr & 0x1e)) {
        iir = AW_UART_IIR_RLSI;
        irq_pending = true;
    }
    else if ((s->ier & AW_UART_IER_RDI) && (s->lsr & AW_UART_LSR_DR)) {
        if (!fifo8_is_empty(&s->rx_fifo) &&
            fifo8_num_used(&s->rx_fifo) >= s->rx_fifo_trigger) {
            iir = AW_UART_IIR_RDI;
            irq_pending = true;
        }
    }
    else if ((s->ier & AW_UART_IER_THRI) && s->thr_ipending) {
        iir = AW_UART_IIR_THRI;
        irq_pending = true;
    }
    else if ((s->ier & AW_UART_IER_MSI) && (s->msr & 0x0F)) {
        iir = AW_UART_IIR_MSI;
        irq_pending = true;
    }

    s->iir = iir | (s->fifo_enabled ? AW_UART_IIR_FIFO_EN : 0);

    qemu_set_irq(s->irq, irq_pending ? 1 : 0);
}

static void aw_uart_receive(void *opaque, const uint8_t *buf, int size)
{
    AwUartState *s = AW_UART(opaque);

    trace("call %s()\n", __func__);

    do {
        if (s->lcr & AW_UART_LCR_DLAB) {
            break;
        }

        for (int i = 0; i < size; i++) {
            if (fifo8_is_full(&s->rx_fifo)) {
                s->lsr |= AW_UART_LSR_OE;
                break;
            }
            fifo8_push(&s->rx_fifo, buf[i]);
        }

        s->lsr |= AW_UART_LSR_DR;
        aw_uart_update_irq(s);
    } while(0);
}

static int aw_uart_can_receive(void *opaque)
{
    AwUartState *s = AW_UART(opaque);

    trace("call %s()\n", __func__);

    return fifo8_num_free(&s->rx_fifo);
}

static void aw_uart_event(void *opaque, QEMUChrEvent event)
{
    trace("call %s()\n", __func__);
}

static void aw_uart_write_tx(AwUartState *s, uint8_t ch)
{
    uint8_t buf = ch;

    printf("%c", ch);
    trace("call %s(ch=%c)\n", __func__, ch);

    s->thr_ipending = false;
    s->lsr &= ~AW_UART_LSR_THRE;

    qemu_chr_fe_write_all(&s->chr, &buf, 1);

    s->thr_ipending = true;
    s->lsr |= AW_UART_LSR_THRE | AW_UART_LSR_TEMT;

    aw_uart_update_irq(s);
}

static uint64_t aw_uart_read(void *opaque, hwaddr addr, unsigned size)
{
    uint64_t r = 0;
    AwUartState *s = AW_UART(opaque);

    trace("call %s()\n", __func__);

    switch (addr) {
    case AW_UART_RBR:
        if (s->lcr & AW_UART_LCR_DLAB) {
            r = s->dll;
        }
        else {
            if (!fifo8_is_empty(&s->rx_fifo)) {
                r = fifo8_pop(&s->rx_fifo);
                if (fifo8_is_empty(&s->rx_fifo)) {
                    s->lsr &= ~AW_UART_LSR_DR;
                }
                qemu_chr_fe_accept_input(&s->chr);
            }
            aw_uart_update_irq(s);
        }
        break;
    case AW_UART_IER:
        if (s->lcr & AW_UART_LCR_DLAB) {
            r = s->dlh;
        }
        else {
            r = s->ier;
        }
        break;
    case AW_UART_IIR:
        r = s->iir;
        if ((s->iir & AW_UART_IIR_ID_MASK) == AW_UART_IIR_THRI) {
            s->thr_ipending = false;
            aw_uart_update_irq(s);
        }
        break;
    case AW_UART_LCR:
        r = s->lcr;
        break;
    case AW_UART_MCR:
        r = s->mcr;
        break;
    case AW_UART_LSR:
        r = s->lsr;
        s->lsr &= ~(AW_UART_LSR_OE | AW_UART_LSR_PE | AW_UART_LSR_FE | AW_UART_LSR_BI);
        aw_uart_update_irq(s);
        break;
    case AW_UART_MSR:
        r = s->msr;
        s->msr &= 0xf0;
        aw_uart_update_irq(s);
        break;
    case AW_UART_SCH:
        r = s->scr;
        break;
    case AW_UART_USR:
        r = AW_UART_USR_TFE | AW_UART_USR_TFNF;
        if (!fifo8_is_empty(&s->rx_fifo)) {
            r |= AW_UART_USR_RFNE;
        }

        if (fifo8_is_full(&s->rx_fifo)) {
            r |= AW_UART_USR_RFF;
        }
        break;
    case AW_UART_TFL:
        r = fifo8_num_used(&s->tx_fifo);
        break;
    case AW_UART_RFL:
        r = fifo8_num_used(&s->rx_fifo);
        break;
    case AW_UART_HALT:
        r = s->halt;
        break;
    default:
        error("invalid offset 0x%lx\n", addr);
        break;
    }

    return r;
}

static void aw_uart_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    AwUartState *s = AW_UART(opaque);

    trace("call %s(addr=0x%lx, val=0x%lx, size=%d)\n", __func__, addr, val, size);

    switch (addr) {
    case AW_UART_THR:
        if (s->lcr & AW_UART_LCR_DLAB) {
            s->dll = val & 0xff;
        }
        else {
            aw_uart_write_tx(s, val & 0xff);
        }
        break;
    case AW_UART_IER:
        if (s->lcr & AW_UART_LCR_DLAB) {
            s->dlh = val & 0xff;
        }
        else {
            s->ier = val & 0x0f;
            aw_uart_update_irq(s);
        }
        break;
    case AW_UART_FCR:
        s->fcr = val & 0xff;
        if (val & AW_UART_FCR_FIFO_EN) {
            s->fifo_enabled = true;
            switch (val & 0xc0) {
            case AW_UART_FCR_TRG_1:
                s->rx_fifo_trigger = 1;
                break;
            case AW_UART_FCR_TRG_4:
                s->rx_fifo_trigger = AW_UART_FIFO_SIZE / 4;
                break;
            case AW_UART_FCR_TRG_8:
                s->rx_fifo_trigger = AW_UART_FIFO_SIZE / 2;
                break;
            case AW_UART_FCR_TRG_14:
                s->rx_fifo_trigger = AW_UART_FIFO_SIZE - 2;
                break;
            }
        }
        else {
            s->fifo_enabled = false;
            s->rx_fifo_trigger = 1;
        }
        if (val & AW_UART_FCR_RX_RST) {
            fifo8_reset(&s->rx_fifo);
            s->lsr &= ~AW_UART_LSR_DR;
        }
        if (val & AW_UART_FCR_TX_RST) {
            fifo8_reset(&s->tx_fifo);
            s->lsr |= AW_UART_LSR_THRE | AW_UART_LSR_TEMT;
            s->thr_ipending = true;
        }
        aw_uart_update_irq(s);
        break;
    case AW_UART_LCR:
        s->lcr = val & 0xff;
        break;
    case AW_UART_MCR:
        s->mcr = val & 0x1f;
        break;
    case AW_UART_LSR:
        break;
    case AW_UART_MSR:
        break;
    case AW_UART_SCH:
        s->scr = val & 0xff;
        break;
    case AW_UART_HALT:
        s->halt = val;
        break;
    default:
        error("invalid offset 0x%lx\n", addr);
        break;
    }
}

static const MemoryRegionOps aw_uart_ops = {
    .read = aw_uart_read,
    .write = aw_uart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4
    }
};

static void aw_uart_reset(DeviceState *dev)
{
    AwUartState *s = AW_UART(dev);

    trace("call %s()\n", __func__);

    s->ier = 0;
    s->iir = AW_UART_IIR_NO_INT;
    s->lcr = 0;
    s->mcr = 0;
    s->lsr = AW_UART_LSR_THRE | AW_UART_LSR_TEMT;
    s->msr = 0;
    s->scr = 0;
    s->dll = 0;
    s->dlh = 0;
    s->fcr = 0;
    s->halt = 0;

    s->fifo_enabled = false;
    s->rx_fifo_trigger = 1;
    s->tx_fifo_trigger = 1;
    s->thr_ipending = true;

    fifo8_reset(&s->rx_fifo);
    fifo8_reset(&s->tx_fifo);
}

static void aw_uart_realize(DeviceState *dev, Error **errp)
{
    AwUartState *s = AW_UART(dev);

    trace("call %s()\n", __func__);

    fifo8_create(&s->rx_fifo, AW_UART_FIFO_SIZE);
    fifo8_create(&s->tx_fifo, AW_UART_FIFO_SIZE);
    qemu_chr_fe_set_handlers(&s->chr, aw_uart_can_receive, aw_uart_receive, aw_uart_event, NULL, s, NULL, true);
}

static void aw_uart_unrealize(DeviceState *dev)
{
    AwUartState *s = AW_UART(dev);

    trace("call %s()\n", __func__);

    fifo8_destroy(&s->rx_fifo);
    fifo8_destroy(&s->tx_fifo);
}

static void aw_uart_init(Object *obj)
{
    AwUartState *s = AW_UART(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    trace("call %s()\n", __func__);

    memory_region_init_io(&s->iomem, obj, &aw_uart_ops, s, TYPE_AW_UART, 0x400);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static Property aw_uart_properties[] = {
    DEFINE_PROP_CHR("chardev", AwUartState, chr),
    DEFINE_PROP_END_OF_LIST()
};

static const VMStateDescription vmstate_aw_uart = {
    .name = TYPE_AW_UART,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT8(ier, AwUartState),
        VMSTATE_UINT8(iir, AwUartState),
        VMSTATE_UINT8(lcr, AwUartState),
        VMSTATE_UINT8(mcr, AwUartState),
        VMSTATE_UINT8(lsr, AwUartState),
        VMSTATE_UINT8(msr, AwUartState),
        VMSTATE_UINT8(scr, AwUartState),
        VMSTATE_UINT8(dll, AwUartState),
        VMSTATE_UINT8(dlh, AwUartState),
        VMSTATE_UINT8(fcr, AwUartState),
        VMSTATE_UINT32(halt, AwUartState),
        VMSTATE_FIFO8(rx_fifo, AwUartState),
        VMSTATE_FIFO8(tx_fifo, AwUartState),
        VMSTATE_UINT8(rx_fifo_trigger, AwUartState),
        VMSTATE_BOOL(fifo_enabled, AwUartState),
        VMSTATE_BOOL(thr_ipending, AwUartState),
        VMSTATE_END_OF_LIST()
    }
};

static void aw_uart_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    trace("call %s()\n", __func__);

    dc->reset = aw_uart_reset;
    dc->realize = aw_uart_realize;
    dc->unrealize = aw_uart_unrealize;
    dc->vmsd = &vmstate_aw_uart;
    device_class_set_props(dc, aw_uart_properties);
}

static const TypeInfo aw_uart_info = {
    .name = TYPE_AW_UART,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AwUartState),
    .instance_init = aw_uart_init,
    .class_init = aw_uart_class_init
};

static void aw_uart_register_types(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&aw_uart_info);
}

type_init(aw_uart_register_types)
