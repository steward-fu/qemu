/*
 * Copyright (C) 2019 Niek Linnenbank <nieklinnenbank@gmail.com>
 * Copyright (C) 2023 Lu Hui <luhux76@gmail.com>
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
#include "hw/misc/f1c100s-ccu.h"

enum {
    REG_PLL_CPU_CTL     = 0x00,
    REG_PLL_AUDIO_CTL   = 0x08,
    REG_PLL_VIDEO_CTL   = 0x10,
    REG_PLL_VE_CTL      = 0x18,
    REG_PLL_DDR_CTL     = 0x20,
    REG_PLL_PERIPH_CTRL = 0x28,
};

static uint64_t allwinner_f1c100s_ccu_read(void *opaque, hwaddr offset, unsigned size)
{
    uint32_t val = 0;
    const AwF1c100sClockCtlState *s = AW_F1C100S_CCU(opaque);

    trace("call %s(offset=0x%lx, size=%d)\n", __func__, offset, size);

    switch (offset) {
    case REG_PLL_CPU_CTL:
    case REG_PLL_AUDIO_CTL:
    case REG_PLL_VIDEO_CTL:
    case REG_PLL_VE_CTL:
    case REG_PLL_DDR_CTL:
    case REG_PLL_PERIPH_CTRL:
        val |= (1 << 28);
        return val;
    default:
        return 0x0;
    }

    return 0;
}

static void allwinner_f1c100s_ccu_write(void *opaque, hwaddr offset,uint64_t val, unsigned size)
{
    AwF1c100sClockCtlState *s = AW_F1C100S_CCU(opaque);

    trace("call %s(offset=0x%lx, val=0x%lx, size=%d)\n", __func__, offset, val, size);
}

static const MemoryRegionOps allwinner_f1c100s_ccu_ops = {
    .read = allwinner_f1c100s_ccu_read,
    .write = allwinner_f1c100s_ccu_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
    .impl.min_access_size = 4,
};

static void allwinner_f1c100s_ccu_reset(DeviceState *dev)
{
    AwF1c100sClockCtlState *s = AW_F1C100S_CCU(dev);

    trace("call %s()\n", __func__);
}

static void allwinner_f1c100s_ccu_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    AwF1c100sClockCtlState *s = AW_F1C100S_CCU(obj);

    trace("call %s()\n", __func__);

    memory_region_init_io(&s->iomem, OBJECT(s), &allwinner_f1c100s_ccu_ops, s, TYPE_AW_F1C100S_CCU, 0x400);
    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription allwinner_f1c100s_ccu_vmstate = {
    .name = "allwinner-f1c100s-ccu",
    .version_id = 1,
    .minimum_version_id = 1,
};

static void allwinner_f1c100s_ccu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    trace("call %s()\n", __func__);

    dc->reset = allwinner_f1c100s_ccu_reset;
    dc->vmsd = &allwinner_f1c100s_ccu_vmstate;
}

static const TypeInfo allwinner_f1c100s_ccu_info = {
    .name = TYPE_AW_F1C100S_CCU,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = allwinner_f1c100s_ccu_init,
    .instance_size = sizeof(AwF1c100sClockCtlState),
    .class_init = allwinner_f1c100s_ccu_class_init,
};

static void allwinner_f1c100s_ccu_register(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&allwinner_f1c100s_ccu_info);
}

type_init(allwinner_f1c100s_ccu_register)
