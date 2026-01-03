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

#define TYPE_AW_F1C100S "f1c100s"
OBJECT_DECLARE_SIMPLE_TYPE(AwF1c100sState, AW_F1C100S)

struct AwF1c100sState {
};

static void aw_f1c100s_realize(DeviceState *dev, Error **errp)
{
    printf("call %s()\n", __func__);
}

static void aw_f1c100s_init(Object *obj)
{
    printf("call %s()\n", __func__);
}

static void aw_f1c100s_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    printf("call %s()\n", __func__);
    dc->realize = aw_f1c100s_realize;
}

static const TypeInfo aw_f1c100s_type_info = {
    .name = "f1c100s",
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(AwF1c100sState),
    .instance_init = aw_f1c100s_init,
    .class_init = aw_f1c100s_class_init,
};

static void aw_f1c100s_register_types(void)
{
    printf("call %s()\n", __func__);
    type_register_static(&aw_f1c100s_type_info);
}

type_init(aw_f1c100s_register_types)

static void aw_f1c100s_board_init(MachineState *machine)
{
    printf("call %s()\n", __func__);
};

static void aw_f1c100s_machine_init(MachineClass *mc)
{
    printf("call %s()\n", __func__);

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
