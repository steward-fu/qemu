#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/datadir.h"
#include "qemu/units.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "hw/ssi/ssi.h"
#include "hw/misc/unimp.h"
#include "hw/boards.h"
#include "hw/usb/hcd-ohci.h"
#include "hw/loader.h"
#include "hw/firmware/smbios.h"
#include "qapi/error.h"
#include "sysemu/sysemu.h"
#include "sysemu/runstate.h"
#include "target/arm/cpu.h"
#include "hw/gpio/f1c100s.h"

#define TYPE_AW_F1C100S "f1c100s"
OBJECT_DECLARE_SIMPLE_TYPE(AwF1c100sState, AW_F1C100S)

enum {
    AW_F1C100S_DEV_SRAM,
    AW_F1C100S_DEV_GPIO,
    AW_F1C100S_DEV_SDRAM,
    AW_F1C100S_DEV_BOOTROM
};

struct AwF1c100sState {
    DeviceState parent_obj;
    ARMCPU cpu;
    const hwaddr *memmap;
    MemoryRegion sram;
    MemoryRegion bootrom;

    AwF1c100sGpioState gpio;
};

static const hwaddr f1c100s_memmap[] = {
    [AW_F1C100S_DEV_SRAM]    = 0x00000000,
    [AW_F1C100S_DEV_GPIO]    = 0x01C20800,
    [AW_F1C100S_DEV_SDRAM]   = 0x80000000,
    [AW_F1C100S_DEV_BOOTROM] = 0xFFFF0000
};

static struct arm_boot_info f1c100s_binfo = { 0 };

static void f1c100s_realize(DeviceState *dev, Error **errp)
{
    AwF1c100sState *s = AW_F1C100S(dev);

    printf("call %s()\n", __func__);

    qdev_realize(DEVICE(&s->cpu), NULL, errp);

    memory_region_init_ram(&s->sram, OBJECT(dev), "sram", 40 * KiB, &error_abort);
    memory_region_add_subregion(get_system_memory(), s->memmap[AW_F1C100S_DEV_SRAM], &s->sram);

    sysbus_realize(SYS_BUS_DEVICE(&s->gpio), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gpio), 0, s->memmap[AW_F1C100S_DEV_GPIO]);
}

static void f1c100s_init(Object *obj)
{
    AwF1c100sState *s = AW_F1C100S(obj);

    printf("call %s()\n", __func__);

    s->memmap = f1c100s_memmap;
    object_initialize_child(obj, "cpu", &s->cpu, ARM_CPU_TYPE_NAME("arm926"));
    object_initialize_child(obj, "gpio", &s->gpio, TYPE_AW_F1C100S_GPIO);
}

static void f1c100s_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    printf("call %s()\n", __func__);
    dc->realize = f1c100s_realize;
}

static const TypeInfo f1c100s_type_info = {
    .name = "f1c100s",
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(AwF1c100sState),
    .instance_init = f1c100s_init,
    .class_init = f1c100s_class_init,
};

static void f1c100s_register_types(void)
{
    printf("call %s()\n", __func__);
    type_register_static(&f1c100s_type_info);
}

type_init(f1c100s_register_types)

static void f1c100s_board_init(MachineState *machine)
{
    AwF1c100sState *f1c100s = NULL;

    printf("call %s()\n", __func__);

    f1c100s = AW_F1C100S(object_new(TYPE_AW_F1C100S));
    object_property_add_child(OBJECT(machine), "soc", OBJECT(f1c100s));
    object_unref(OBJECT(f1c100s));

    qdev_realize(DEVICE(f1c100s), NULL, &error_abort);
    memory_region_add_subregion(get_system_memory(), f1c100s->memmap[AW_F1C100S_DEV_SDRAM], machine->ram);
    memory_region_init_rom(&f1c100s->bootrom, NULL, "aw_f1c100s.bootrom", 64 * KiB, &error_fatal);
    memory_region_add_subregion(get_system_memory(), f1c100s->memmap[AW_F1C100S_DEV_BOOTROM], &f1c100s->bootrom);

    char *filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, machine->firmware);
    if (filename) {
        printf("loading... \"%s\"\n", filename);
        load_image_targphys(filename, f1c100s->memmap[AW_F1C100S_DEV_BOOTROM], 64 * KiB);
        g_free(filename);

        f1c100s_binfo.entry = f1c100s->memmap[AW_F1C100S_DEV_BOOTROM];
    }

    f1c100s_binfo.ram_size = machine->ram_size;
    CPUARMState *env = &f1c100s->cpu.env;
    env->boot_info = &f1c100s_binfo;
    arm_load_kernel(&f1c100s->cpu, machine, &f1c100s_binfo);
};

static void f1c100s_machine_init(MachineClass *mc)
{
    printf("call %s()\n", __func__);

    mc->desc = "Allwinner F1C100S (ARM926EJ-S)";
    mc->init = f1c100s_board_init;
    mc->min_cpus = 1;
    mc->max_cpus = 1;
    mc->default_cpus = 1;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("arm926");
    mc->default_ram_size = 32 * MiB;
    mc->default_ram_id = "aw_f1c100s.ram";
};

DEFINE_MACHINE("f1c100s", f1c100s_machine_init)
