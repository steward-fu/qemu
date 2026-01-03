/*
 * Copyright (C) 2018 Mesih Kilinc <mesihkilinc@gmail.com>
 * Copyright (C) 2018 Icenowy Zheng <icenowy@aosc.io>
 * Copyright (C) 2013 Li Guang
 * Copyright (C) 2023 Lu Hui <luhux76@gmail.com>
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
#include "qemu/log.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "qemu/datadir.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "hw/arm/f1c100s.h"
#include "hw/misc/f1c100s-ccu.h"
#include "hw/misc/allwinner-sid.h"
#include "hw/intc/f1c100s.h"
#include "hw/gpio/f1c100s.h"
#include "hw/timer/allwinner-a10-pit.h"
#include "hw/ssi/allwinner-sun6i-spi.h"
#include "hw/ssi/ssi.h"
#include "hw/misc/unimp.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "hw/usb/hcd-ohci.h"
#include "hw/loader.h"
#include "qemu/units.h"
#include "qemu/aw_log.h"
#include "hw/firmware/smbios.h"
#include "sysemu/runstate.h"

struct aw_shm_t aw_shm = { 0 };

int aw_dev_mode = 0;
int aw_debug_level = FATAL_LEVEL;
struct arm_boot_info f1c100s_binfo = { 0 };

const hwaddr allwinner_f1c100s_memmap[] = {
    [AW_F1C100S_DEV_SRAM_A1]    = 0x00000000,
    [AW_F1C100S_DEV_SPI0]       = 0x01C05000,
    [AW_F1C100S_DEV_SPI1]       = 0x01C06000,
    [AW_F1C100S_DEV_CCU]        = 0x01C20000,
    [AW_F1C100S_DEV_INTC]       = 0x01C20400,
    [AW_F1C100S_DEV_GPIO]       = 0x01C20800,
    [AW_F1C100S_DEV_TIMER]      = 0x01C20C00,
    [AW_F1C100S_DEV_SID]        = 0x01C23800,
    [AW_F1C100S_DEV_MMC0]       = 0x01C0F000,
    [AW_F1C100S_DEV_MMC1]       = 0x01C10000,
    [AW_F1C100S_DEV_UART0]      = 0x01C25000,
    [AW_F1C100S_DEV_UART1]      = 0x01C25400,
    [AW_F1C100S_DEV_UART2]      = 0x01C25800,
    [AW_F1C100S_DEV_TWI0]       = 0x01C27000,
    [AW_F1C100S_DEV_TWI1]       = 0x01C27400,
    [AW_F1C100S_DEV_TWI2]       = 0x01C27800,
    [AW_F1C100S_DEV_LOG_BUF]    = 0x4F000000,
    [AW_F1C100S_DEV_SDRAM]      = 0x80000000,
    [AW_F1C100S_DEV_BOOTROM]    = 0xFFFF0000,
};

static struct AwF1C100SUnimplemented {
    const char *device_name;
    hwaddr base;
    hwaddr size;
} unimplemented[] = {
    { "sysctrl", 0x01C00000, 4 * KiB },
    { "dramc",   0x01C01000, 4 * KiB },
    { "dma",     0x01C02000, 4 * KiB },
    { "tve",     0x01C0A000, 4 * KiB },
    { "tvd",     0x01C0B000, 4 * KiB },
    { "tcon",    0x01C0C000, 4 * KiB },
    { "ve",      0x01C0E000, 4 * KiB },
    { "otg",     0x01C13000, 4 * KiB },
    { "pio",     0x01C20800, 1 * KiB },
    { "pwm",     0x01C21000, 1 * KiB },
    { "owa",     0x01C21400, 1 * KiB },
    { "rsb",     0x01C21800, 1 * KiB },
    { "daudio",  0x01C22000, 1 * KiB },
    { "cir",     0x01C22C00, 1 * KiB },
    { "keyadc",  0x01C23400, 1 * KiB },
    { "audio",   0x01C23C00, 1 * KiB },
    { "tp",      0x01C24800, 1 * KiB },
    { "csi",     0x01CB0000, 4 * KiB },
    { "defe",    0x01E00000, 128 * KiB },
    { "debe",    0x01E60000, 64 * KiB },
    { "dei",     0x01E70000, 64 * KiB },
};

enum {
    IRQ_UART0  = 1,
    IRQ_UART1  = 2,
    IRQ_UART2  = 3,
    IRQ_TWI0   = 7,
    IRQ_TWI1   = 8,
    IRQ_TWI2   = 9,
    IRQ_TIMER0 = 13,
    IRQ_TIMER1 = 14,
    IRQ_TIMER2 = 15,
    IRQ_WDOG   = 16,
    IRQ_MMC0   = 23,
    IRQ_MMC1   = 24,
};

static void aw_f1c100s_init(Object *obj)
{
    AwF1c100sState *s = AW_F1C100S(obj);

    trace("call %s()\n", __func__);

    s->memmap = allwinner_f1c100s_memmap;
    object_initialize_child(obj, "cpu", &s->cpu, ARM_CPU_TYPE_NAME("arm926"));
    object_initialize_child(obj, "spi[0]", &s->spi[0], TYPE_AW_SUN6I_SPI);
    object_initialize_child(obj, "spi[1]", &s->spi[1], TYPE_AW_SUN6I_SPI);
    object_initialize_child(obj, "ccu", &s->ccu, TYPE_AW_F1C100S_CCU);
    object_initialize_child(obj, "intc", &s->intc, TYPE_AW_F1C100S_INTC);
    object_initialize_child(obj, "gpio", &s->gpio, TYPE_AW_F1C100S_GPIO);
    object_initialize_child(obj, "timer", &s->timer, TYPE_AW_A10_PIT);
    object_initialize_child(obj, "sid", &s->sid, TYPE_AW_SID);
    object_initialize_child(obj, "mmc[0]", &s->mmc[0], TYPE_AW_SDHOST_SUN5I);
    object_initialize_child(obj, "mmc[1]", &s->mmc[1], TYPE_AW_SDHOST_SUN5I);
    object_initialize_child(obj, "i2c[0]", &s->i2c[0], TYPE_AW_I2C);
    object_initialize_child(obj, "i2c[1]", &s->i2c[1], TYPE_AW_I2C);
    object_initialize_child(obj, "i2c[2]", &s->i2c[2], TYPE_AW_I2C);
    object_initialize_child(obj, "uart[0]", &s->uart[0], TYPE_AW_UART);
    object_initialize_child(obj, "uart[1]", &s->uart[1], TYPE_AW_UART);
    object_initialize_child(obj, "uart[2]", &s->uart[2], TYPE_AW_UART);
    object_property_add_alias(obj, "identifier", OBJECT(&s->sid), "identifier");
    object_property_add_alias(obj, "clk0-freq", OBJECT(&s->timer), "clk0-freq");
    object_property_add_alias(obj, "clk1-freq", OBJECT(&s->timer), "clk1-freq");
}

static void aw_shutdown_cb(Notifier *n, void *opaque)
{
    trace("call %s()\n", __func__);
}

static void aw_f1c100s_realize(DeviceState *dev, Error **errp)
{
    int i = 0;
    AwF1c100sState *s = AW_F1C100S(dev);

    trace("call %s()\n", __func__);

    if (!qdev_realize(DEVICE(&s->cpu), NULL, errp)) {
        error("failed to realize CPU\n");
        return;
    }

    memory_region_init_ram(&s->sram_a1, OBJECT(dev), "sram a1", 40 * KiB, &error_abort);
    memory_region_add_subregion(get_system_memory(), s->memmap[AW_F1C100S_DEV_SRAM_A1], &s->sram_a1);
    memory_region_init_ram(&s->sram_logbuf, OBJECT(dev), "sram logbuf", 4096, &error_abort);
    memory_region_add_subregion(get_system_memory(), s->memmap[AW_F1C100S_DEV_LOG_BUF], &s->sram_logbuf);

    sysbus_realize(SYS_BUS_DEVICE(&s->ccu), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->ccu), 0, s->memmap[AW_F1C100S_DEV_CCU]);

    sysbus_realize(SYS_BUS_DEVICE(&s->gpio), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gpio), 0, s->memmap[AW_F1C100S_DEV_GPIO]);

    sysbus_realize(SYS_BUS_DEVICE(&s->intc), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->intc), 0, s->memmap[AW_F1C100S_DEV_INTC]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->intc), 0, qdev_get_gpio_in(DEVICE(&s->cpu), ARM_CPU_IRQ));
    qdev_pass_gpios(DEVICE(&s->intc), dev, NULL);

    sysbus_realize(SYS_BUS_DEVICE(&s->timer), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timer), 0, s->memmap[AW_F1C100S_DEV_TIMER]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 0, qdev_get_gpio_in(dev, IRQ_TIMER0));
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 1, qdev_get_gpio_in(dev, IRQ_TIMER1));
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 2, qdev_get_gpio_in(dev, IRQ_TIMER2));
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 3, qdev_get_gpio_in(dev, IRQ_WDOG));

    sysbus_realize(SYS_BUS_DEVICE(&s->sid), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sid), 0, s->memmap[AW_F1C100S_DEV_SID]);

    object_property_set_link(OBJECT(&s->mmc[0]), "dma-memory", OBJECT(get_system_memory()), &error_fatal);
    sysbus_realize(SYS_BUS_DEVICE(&s->mmc[0]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mmc[0]), 0, s->memmap[AW_F1C100S_DEV_MMC0]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->mmc[0]), 0, qdev_get_gpio_in(DEVICE(dev), IRQ_MMC0));
    object_property_add_alias(OBJECT(s), "sd-bus[0]", OBJECT(&s->mmc[0]), "sd-bus");

    object_property_set_link(OBJECT(&s->mmc[1]), "dma-memory", OBJECT(get_system_memory()), &error_fatal);
    sysbus_realize(SYS_BUS_DEVICE(&s->mmc[1]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mmc[1]), 0, s->memmap[AW_F1C100S_DEV_MMC1]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->mmc[1]), 0, qdev_get_gpio_in(DEVICE(dev), IRQ_MMC1));

    object_property_add_alias(OBJECT(s), "sd-bus[1]", OBJECT(&s->mmc[1]), "sd-bus");

    AwSun6iSpiState *spi_bus = &s->spi[0];
    sysbus_realize(SYS_BUS_DEVICE(spi_bus), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(spi_bus), 0, s->memmap[AW_F1C100S_DEV_SPI0]);

    DriveInfo *dinfo = drive_get(IF_MTD, 0, 0);
    DeviceState *spi_flash = NULL;
    qemu_irq cs_line = { 0 };

    if (dinfo) {
        spi_flash = qdev_new("w25q64");
        qdev_prop_set_drive(spi_flash, "drive", blk_by_legacy_dinfo(dinfo));
        qdev_realize_and_unref(spi_flash, BUS(spi_bus->spi), &error_fatal);
        cs_line = qdev_get_gpio_in_named(spi_flash, SSI_GPIO_CS, 0);
        sysbus_connect_irq(SYS_BUS_DEVICE(spi_bus), 0, cs_line);
    }

    sysbus_realize(SYS_BUS_DEVICE(&s->spi[1]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi[1]), 0, s->memmap[AW_F1C100S_DEV_SPI1]);

    for (i = 0; i < 3; i++) {
        qdev_prop_set_chr(DEVICE(&s->uart[i]), "chardev", serial_hd(i));
        sysbus_realize(SYS_BUS_DEVICE(&s->uart[i]), &error_fatal);
        sysbus_mmio_map(SYS_BUS_DEVICE(&s->uart[i]), 0, s->memmap[AW_F1C100S_DEV_UART0 + i]);
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->uart[i]), 0, qdev_get_gpio_in(dev, IRQ_UART0 + i));
    }

    sysbus_realize(SYS_BUS_DEVICE(&s->i2c[0]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c[0]), 0, s->memmap[AW_F1C100S_DEV_TWI0]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->i2c[0]), 0, qdev_get_gpio_in(dev, IRQ_TWI0));

    sysbus_realize(SYS_BUS_DEVICE(&s->i2c[1]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c[1]), 0, s->memmap[AW_F1C100S_DEV_TWI1]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->i2c[1]), 0, qdev_get_gpio_in(dev, IRQ_TWI1));

    sysbus_realize(SYS_BUS_DEVICE(&s->i2c[2]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c[2]), 0, s->memmap[AW_F1C100S_DEV_TWI2]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->i2c[2]), 0, qdev_get_gpio_in(dev, IRQ_TWI2));

    s->shutdown_notifier.notify = aw_shutdown_cb;
    qemu_register_shutdown_notifier(&s->shutdown_notifier);

    for (i = 0; i < ARRAY_SIZE(unimplemented); i++) {
        create_unimplemented_device(unimplemented[i].device_name, unimplemented[i].base, unimplemented[i].size);
    }

    const char *env = getenv("QEMU_DEV_MODE");
    if (env && !strcmp(env, "1")) {
        trace("dev mode\n");
        aw_dev_mode = 1;
    }
}

static void aw_f1c100s_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    trace("call %s()\n", __func__);

    dc->realize = aw_f1c100s_realize;
}

static const TypeInfo aw_f1c100s_type_info = {
    .name = TYPE_AW_F1C100S,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(AwF1c100sState),
    .instance_init = aw_f1c100s_init,
    .class_init = aw_f1c100s_class_init,
};

static void aw_f1c100s_register_types(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&aw_f1c100s_type_info);
}

type_init(aw_f1c100s_register_types)

static void aw_f1c100s_board_init(MachineState *machine)
{
    char *filename = NULL;
    AwF1c100sState *f1c100s;

    trace("call %s()\n", __func__);

    if (strcmp(machine->cpu_type, ARM_CPU_TYPE_NAME("arm926")) != 0) {
        fatal("only arm926ejs supported\n");
        exit(1);
    }

    if (machine->ram_size > 64 * MiB) {
        fatal("only 64MB supported\n");
        exit(1);
    }

    f1c100s = AW_F1C100S(object_new(TYPE_AW_F1C100S));
    object_property_add_child(OBJECT(machine), "soc", OBJECT(f1c100s));
    object_unref(OBJECT(f1c100s));

    object_property_set_int(OBJECT(f1c100s), "clk0-freq", 32768, &error_abort);
    object_property_set_int(OBJECT(f1c100s), "clk1-freq", 24 * 1000 * 1000, &error_abort);

    if (qemu_uuid_is_null(&f1c100s->sid.identifier)) {
        qdev_prop_set_string(DEVICE(f1c100s), "identifier", "00000000-1111-2222-3333-000044556677");
    }

    qdev_realize(DEVICE(f1c100s), NULL, &error_abort);

    memory_region_add_subregion(get_system_memory(), f1c100s->memmap[AW_F1C100S_DEV_SDRAM], machine->ram);

    if (machine->ram_size < (2 * GiB)) {
        create_unimplemented_device("sdram[unused]", f1c100s->memmap[AW_F1C100S_DEV_SDRAM] + machine->ram_size, (2 * GiB) - machine->ram_size);
    }

    memory_region_init_rom(&f1c100s->bootrom, NULL, "aw_f1c100s.bootrom", 64 * KiB, &error_fatal);
    memory_region_add_subregion(get_system_memory(), f1c100s->memmap[AW_F1C100S_DEV_BOOTROM], &f1c100s->bootrom);

    filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, machine->firmware);
    if (filename) {
        load_image_targphys(filename, f1c100s->memmap[AW_F1C100S_DEV_BOOTROM], 64 * KiB);
        g_free(filename);
        f1c100s_binfo.entry = f1c100s->memmap[AW_F1C100S_DEV_BOOTROM];
    }
    f1c100s_binfo.ram_size = machine->ram_size;
    CPUARMState *env = &f1c100s->cpu.env;
    env->boot_info = &f1c100s_binfo;
    arm_load_kernel(&f1c100s->cpu, machine, &f1c100s_binfo);
};

static void aw_f1c100s_machine_init(MachineClass *mc)
{
    trace("call %s()\n", __func__);

    mc->desc = "Allwinner F1C100S (ARM926EJ-S)";
    mc->init = aw_f1c100s_board_init;
    mc->min_cpus = 1;
    mc->max_cpus = 1;
    mc->default_cpus = 1;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("arm926");
    mc->default_ram_size = 32 * MiB;
    mc->default_ram_id = "aw_f1c100s.ram";
};

DEFINE_MACHINE("f1c100s", aw_f1c100s_machine_init)
