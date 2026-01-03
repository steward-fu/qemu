/*
 * Copyright (C) 2025 Steward <steward.fu@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/aw_log.h"
#include "qemu/module.h"
#include "hw/gpio/f1c100s.h"

static uint64_t f1c100s_read(void *opaque, hwaddr offset, unsigned size)
{
    trace("call %s(offset=0x%lx, size=%d)\n", __func__, offset, size);

    return 0;
}

static void f1c100s_write(void *opaque, hwaddr offset, uint64_t val, unsigned size)
{
    
    int idx = 0;

    trace("call %s(offset=0x%lx, val=0x%lx, size=%d)\n", __func__, offset, val, size);

    switch (offset) {
    case PE_CFG0:
        for () {
        }
        break;
    case PE_DATA:
        aw_shm.pe.data = val & asm_shm.pe.dir;
        break;
    }
}

static const MemoryRegionOps f1c100s_ops = {
    .read = f1c100s_read,
    .write = f1c100s_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
    .impl.min_access_size = 4,
};

static void f1c100s_reset(DeviceState *dev)
{
    trace("call %s()\n", __func__);
}

static void f1c100s_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    AwF1c100sGpioState *s = AW_F1C100S_GPIO(obj);

    trace("call %s()\n", __func__);

    memory_region_init_io(&s->iomem, OBJECT(s), &f1c100s_ops, s, TYPE_AW_F1C100S_GPIO, 0x400);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void f1c100s_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    trace("call %s()\n", __func__);

    dc->reset = f1c100s_reset;
}

static const TypeInfo f1c100s_info = {
    .name = TYPE_AW_F1C100S_GPIO,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = f1c100s_init,
    .instance_size = sizeof(AwF1c100sGpioState),
    .class_init = f1c100s_class_init,
};

static void f1c100s_register(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&f1c100s_info);
}

type_init(f1c100s_register)
