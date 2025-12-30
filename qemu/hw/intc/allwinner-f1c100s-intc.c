/*
 * Copyright (C) 2013 Li Guang
 * Copyright (C) 2023 Lu Hui <luhux76@gmail.com>
 * Copyright (C) 2023 zhaosx <shaoxi2010@qq.com>
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

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "hw/intc/allwinner-f1c100s-intc.h"
#include "hw/irq.h"
#include "qemu/aw_log.h"
#include "qemu/module.h"

enum {
    REG_VECTOR    = 0x00,
    REG_BASE_ADDR = 0x04,
    REG_NMI_CTL   = 0x0C,
    REG_PENDING0  = 0x10,
    REG_PENDING1  = 0x14,
    REG_ENABLE0   = 0x20,
    REG_ENABLE1   = 0x24,
    REG_MASK0     = 0x30,
    REG_MASK1     = 0x34,
};

static void aw_f1c100s_intc_update(AwF1c100sIntcState *s)
{
    int i = 0;
    int irq = 0;
    int zeroes = 0;
    int irq_trigger = 0;

    trace("call %s()\n", __func__);

    s->vector = 0;
    for (i = 0 ; i < 2; i++) {
        irq = ~s->mask[i] & s->enable[i];
        if (!s->vector) {
            zeroes = ctz32(irq & s->pending[i]);
            if ((zeroes != 32) && (irq_trigger == 0) ) {
                s->vector = (i * 32 + zeroes) * 4;
                s->pending[i] |= ~(0x1 << zeroes);
                irq_trigger = 1;
            }
        }
    }
    qemu_set_irq(s->parent_irq, !!irq_trigger);
}

static void aw_f1c100s_intc_set_irq(void *opaque, int irq, int level)
{
    AwF1c100sIntcState *s = opaque;

    trace("call %s(irq=%d, level=%d)\n", __func__, irq, level);

    if (level) {
        set_bit(irq % 32, (void *)&s->pending[irq / 32]);
    }
    else {
        clear_bit(irq % 32, (void *)&s->pending[irq / 32]);
    }

    aw_f1c100s_intc_update(s);
}

static uint64_t aw_f1c100s_intc_read(void *opaque, hwaddr offset, unsigned size)
{
    AwF1c100sIntcState *s = opaque;

    trace("call %s(offset=0x%lx, size=%d)\n", __func__, offset, size);

    switch (offset) {
    case REG_VECTOR:
        return s->vector;
    case REG_BASE_ADDR:
        return s->base_addr;
    case REG_NMI_CTL:
        return s->nmi_ctl;
    case REG_PENDING0:
        return s->pending[0];
    case REG_PENDING1:
        return s->pending[1];
    case REG_ENABLE0:
        return s->enable[0];
    case REG_ENABLE1:
        return s->enable[1];
    case REG_MASK0:
        return s->mask[0];
    case REG_MASK1:
        return s->mask[1];
    default:
        error("invalid offset: 0x%lx\n", offset);
        return 0;
    }

    return 0;
}

static void aw_f1c100s_intc_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    AwF1c100sIntcState *s = opaque;

    trace("call %s(offset=0x%lx, value=0x%lx, size=%d)\n", __func__, offset, value, size);

    switch (offset) {
    case REG_BASE_ADDR:
        s->base_addr = value & ~0x3;
        break;
    case REG_NMI_CTL:
        s->nmi_ctl = value;
        break;
    case REG_PENDING0:
        s->pending[0] = ~value;
        break;
    case REG_PENDING1:
        s->pending[1] = ~value;
        break;
    case REG_ENABLE0:
        s->enable[0] = value;
        break;
    case REG_ENABLE1:
        s->enable[1] = value;
        break;
    case REG_MASK0:
        s->mask[0] = value;
        break;
    case REG_MASK1:
        s->mask[1] = value;
        break;
    default:
        error("invalid offset: 0x%lx\n", offset);
        break;
    }

    aw_f1c100s_intc_update(s);
}

static const MemoryRegionOps aw_f1c100s_intc_ops = {
    .read = aw_f1c100s_intc_read,
    .write = aw_f1c100s_intc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_aw_f1c100s_intc = {
    .name = "allwinner-f1c100s-intc",
    .version_id = 1,
    .minimum_version_id = 1,
};

static void aw_f1c100s_intc_init(Object *obj)
{
    SysBusDevice *dev = SYS_BUS_DEVICE(obj);
    AwF1c100sIntcState *s = AW_F1C100S_INTC(obj);

    trace("call %s()\n", __func__);

    /* f1c100s have 41 irq */
    qdev_init_gpio_in(DEVICE(dev), aw_f1c100s_intc_set_irq, 41);
    sysbus_init_irq(dev, &s->parent_irq);
    memory_region_init_io(&s->iomem, OBJECT(s), &aw_f1c100s_intc_ops, s, TYPE_AW_F1C100S_INTC, 0x400);
    sysbus_init_mmio(dev, &s->iomem);
}

static void aw_f1c100s_intc_reset(DeviceState *d)
{
    uint8_t i = 0;
    AwF1c100sIntcState *s = AW_F1C100S_INTC(d);

    trace("call %s()\n", __func__);

    s->base_addr = 0;
    s->nmi_ctl = 0;
    s->vector = 0;
    for (i = 0; i < 2; i++) {
        s->pending[i] = 0;
        s->enable[i] = 0;
        s->mask[i] = 0;
    }
}

static void aw_f1c100s_intc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    trace("call %s()\n", __func__);

    dc->reset = aw_f1c100s_intc_reset;
    dc->desc = "allwinner f1c100s intc";
    dc->vmsd = &vmstate_aw_f1c100s_intc;
 }

static const TypeInfo aw_f1c100s_intc_info = {
    .name = TYPE_AW_F1C100S_INTC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AwF1c100sIntcState),
    .instance_init = aw_f1c100s_intc_init,
    .class_init = aw_f1c100s_intc_class_init,
};

static void aw_a10_register_types(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&aw_f1c100s_intc_info);
}

type_init(aw_a10_register_types);
