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
#include "hw/char/f1c100s_uart.h"
#include "hw/timer/f1c100s.h"
#include "hw/ssi/f1c100s.h"

#define TYPE_F1C100S "f1c100s"
OBJECT_DECLARE_SIMPLE_TYPE(f1c100s_soc_state, F1C100S)

int f1c100s_debug_level = TRACE_LEVEL;

enum {
    SRAM_BASE,
    CCU_BASE,
    INTC_BASE,
    GPIO_BASE,
    TIMER_BASE,
    SDRAM_BASE,
    UART0_BASE,
    UART1_BASE,
    UART2_BASE,
    SPI0_BASE,
    SPI1_BASE,
    BOOTROM_BASE
};

struct f1c100s_soc_state {
    DeviceState parent_obj;
    ARMCPU cpu;
    const hwaddr *memmap;
    MemoryRegion sram;
    MemoryRegion bootrom;

    f1c100s_ccu_state ccu;
    f1c100s_spi_state spi[2];
    f1c100s_intc_state intc;
    f1c100s_gpio_state gpio;
    f1c100s_timer_state timer;
    f1c100s_uart_state uart[3];
};

static const hwaddr f1c100s_memmap[] = {
    [SRAM_BASE]    = 0x00000000,
    [CCU_BASE]     = 0x01c20000,
    [INTC_BASE]    = 0x01c20400,
    [GPIO_BASE]    = 0x01c20800,
    [TIMER_BASE]   = 0x01c20c00,
    [SDRAM_BASE]   = 0x80000000,
    [UART0_BASE]   = 0x01c25000,
    [UART1_BASE]   = 0x01c25400,
    [UART2_BASE]   = 0x01c25800,
    [SPI0_BASE]    = 0x01c05000,
    [SPI1_BASE]    = 0x01c06000,
    [BOOTROM_BASE] = 0xffff0000
};

enum {
    IRQ_TIMER0 = 13,
    IRQ_TIMER1 = 14,
    IRQ_TIMER2 = 15,
};

static struct arm_boot_info f1c100s_binfo = { 0 };
 
static void f1c100s_soc_realize(DeviceState *dev, Error **errp)
{
    int i = 0;
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

    sysbus_realize(SYS_BUS_DEVICE(&s->timer), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timer), 0, s->memmap[TIMER_BASE]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 0, qdev_get_gpio_in(DEVICE(&s->intc), IRQ_TIMER0));
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 1, qdev_get_gpio_in(DEVICE(&s->intc), IRQ_TIMER1));
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 2, qdev_get_gpio_in(DEVICE(&s->intc), IRQ_TIMER2));

    for (i = 0; i < 3; i++) {
        qdev_prop_set_chr(DEVICE(&s->uart[i]), "chardev", serial_hd(i));
        sysbus_realize(SYS_BUS_DEVICE(&s->uart[i]), &error_fatal);
        sysbus_mmio_map(SYS_BUS_DEVICE(&s->uart[i]), 0, s->memmap[UART0_BASE + i]);
    }

    sysbus_realize(SYS_BUS_DEVICE(&s->spi[0]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi[0]), 0, s->memmap[SPI0_BASE]);

    sysbus_realize(SYS_BUS_DEVICE(&s->spi[1]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi[1]), 0, s->memmap[SPI1_BASE]);

    DriveInfo *dinfo = drive_get(IF_MTD, 0, 0);
    if (dinfo) {
        DeviceState *spi_flash;
        BusState *spi_bus;

        spi_flash = qdev_new("w25q64");
        qdev_prop_set_drive(spi_flash, "drive", blk_by_legacy_dinfo(dinfo));
        spi_bus = qdev_get_child_bus(DEVICE(&s->spi[0]), "ssi");
        qdev_realize_and_unref(spi_flash, spi_bus, &error_fatal);

        qemu_irq cs_line = qdev_get_gpio_in_named(spi_flash, SSI_GPIO_CS, 0);
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->spi[0]), 0, cs_line);
    }
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
    object_initialize_child(obj, "timer", &s->timer, TYPE_F1C100S_TIMER);
    object_initialize_child(obj, "uart0", &s->uart[0], TYPE_F1C100S_UART);
    object_initialize_child(obj, "uart1", &s->uart[1], TYPE_F1C100S_UART);
    object_initialize_child(obj, "uart2", &s->uart[2], TYPE_F1C100S_UART);
    object_initialize_child(obj, "spi[0]", &s->spi[0], TYPE_F1C100S_SPI);
    object_initialize_child(obj, "spi[1]", &s->spi[1], TYPE_F1C100S_SPI);
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
