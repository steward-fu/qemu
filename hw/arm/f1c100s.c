#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/datadir.h"
#include "qemu/units.h"
#include "qemu/f1c100s_log.h"
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
#include "hw/misc/f1c100s_ccu.h"
#include "hw/intc/f1c100s.h"

#define TYPE_F1C100S "f1c100s"
OBJECT_DECLARE_SIMPLE_TYPE(f1c100s_soc_state, F1C100S)

int f1c100s_debug_level = TRACE_LEVEL;

enum {
    SRAM_BASE,
    CCU_BASE,
    INTC_BASE,
    GPIO_BASE,
    SDRAM_BASE,
    BOOTROM_BASE
};

struct f1c100s_soc_state {
    DeviceState parent_obj;
    ARMCPU cpu;
    const hwaddr *memmap;
    MemoryRegion sram;
    MemoryRegion bootrom;

    f1c100s_ccu_state ccu;
    f1c100s_intc_state intc;
    f1c100s_gpio_state gpio;
};

static const hwaddr f1c100s_memmap[] = {
    [SRAM_BASE]    = 0x00000000,
    [CCU_BASE]     = 0x01c20000,
    [INTC_BASE]    = 0x01c20400,
    [GPIO_BASE]    = 0x01c20800,
    [SDRAM_BASE]   = 0x80000000,
    [BOOTROM_BASE] = 0xffff0000
};
 
static struct arm_boot_info f1c100s_binfo = { 0 };
 
static void f1c100s_soc_realize(DeviceState *dev, Error **errp)
{
    f1c100s_soc_state *s = F1C100S(dev);

    trace("call %s()\n", __func__);
    qdev_realize(DEVICE(&s->cpu), NULL, errp);

    memory_region_init_ram(&s->sram, OBJECT(dev), "sram", 40 * KiB, &error_abort);
    memory_region_add_subregion(get_system_memory(), s->memmap[SRAM_BASE], &s->sram);

    sysbus_realize(SYS_BUS_DEVICE(&s->ccu), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->ccu), 0, s->memmap[CCU_BASE]);

    sysbus_realize(SYS_BUS_DEVICE(&s->intc), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->intc), 0, s->memmap[INTC_BASE]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->intc), 0, qdev_get_gpio_in(DEVICE(&s->cpu), ARM_CPU_IRQ));

    sysbus_realize(SYS_BUS_DEVICE(&s->gpio), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gpio), 0, s->memmap[GPIO_BASE]);
}
 
static void f1c100s_soc_instance_init(Object *obj)
{
    f1c100s_soc_state *s = F1C100S(obj);

    trace("call %s()\n", __func__);
    s->memmap = f1c100s_memmap;
    object_initialize_child(obj, "cpu", &s->cpu, ARM_CPU_TYPE_NAME("f1c100s"));
    object_initialize_child(obj, "ccu", &s->ccu, TYPE_F1C100S_CCU);
    object_initialize_child(obj, "intc", &s->intc, TYPE_F1C100S_INTC);
    object_initialize_child(obj, "gpio", &s->gpio, TYPE_F1C100S_GPIO);
}
 
static void f1c100s_soc_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
 
    trace("call %s()\n", __func__);
    dc->realize = f1c100s_soc_realize;
}
 
static const TypeInfo f1c100s_soc_type_info = {
    .name = "f1c100s",
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(f1c100s_soc_state),
    .instance_init = f1c100s_soc_instance_init,
    .class_init = f1c100s_soc_class_init,
};
 
static void f1c100s_soc_register_types(void)
{
    trace("call %s()\n", __func__);
    type_register_static(&f1c100s_soc_type_info);
}
 
type_init(f1c100s_soc_register_types)
 
static void f1c100s_soc_board_init(MachineState *machine)
{
    f1c100s_soc_state *s = NULL;

    trace("call %s()\n", __func__);
    s = F1C100S(object_new(TYPE_F1C100S));
    object_property_add_child(OBJECT(machine), "soc", OBJECT(s));
    object_unref(OBJECT(s));
 
    qdev_realize(DEVICE(s), NULL, &error_abort);
    memory_region_add_subregion(get_system_memory(), s->memmap[SDRAM_BASE], machine->ram);
    memory_region_init_rom(&s->bootrom, NULL, "f1c100s.bootrom", 64 * KiB, &error_fatal);
    memory_region_add_subregion(get_system_memory(), s->memmap[BOOTROM_BASE], &s->bootrom);
 
    char *fname = qemu_find_file(QEMU_FILE_TYPE_BIOS, machine->firmware);
    if (fname) {
        trace("loading... \"%s\"\n", fname);
        load_image_targphys(fname, s->memmap[BOOTROM_BASE], 64 * KiB);
        g_free(fname);
 
        f1c100s_binfo.entry = s->memmap[BOOTROM_BASE];
    }
 
    f1c100s_binfo.ram_size = machine->ram_size;
    CPUARMState *env = &s->cpu.env;
    env->boot_info = &f1c100s_binfo;
    arm_load_kernel(&s->cpu, machine, &f1c100s_binfo);
};
 
static void f1c100s_soc_init(MachineClass *mc)
{
    trace("call %s()\n", __func__);
 
    mc->desc = "Allwinner F1C100S (ARM926EJ-S)";
    mc->init = f1c100s_soc_board_init;
    mc->min_cpus = 1;
    mc->max_cpus = 1;
    mc->default_cpus = 1;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("f1c100s");
    mc->default_ram_size = 32 * MiB;
    mc->default_ram_id = "f1c100s.ram";
};
 
DEFINE_MACHINE("f1c100s", f1c100s_soc_init)
