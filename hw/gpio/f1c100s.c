#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "migration/vmstate.h"
#include "hw/sysbus.h"
#include "hw/gpio/f1c100s.h"

static uint64_t f1c100s_read(void *opaque, hwaddr offset, unsigned size)
{
    printf("call %s(offset=0x%lx, size=%d)\n", __func__, offset, size);

    return 0;
}

static void f1c100s_write(void *opaque, hwaddr offset, uint64_t val, unsigned size)
{
    printf("call %s(offset=0x%lx, val=0x%lx, size=%d)\n", __func__, offset, val, size);
}

static const MemoryRegionOps f1c100s_ops = {
    .read = f1c100s_read,
    .write = f1c100s_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void f1c100s_gpio_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    AwF1c100sGpioState *s = AW_F1C100S_GPIO(dev);

    printf("call %s()\n", __func__);

    memory_region_init_io(&s->iomem, OBJECT(s), &f1c100s_ops, s, TYPE_AW_F1C100S_GPIO, 0x400);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void f1c100s_init(Object *obj)
{
    printf("call %s()\n", __func__);
}

static void f1c100s_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    printf("call %s()\n", __func__);
    dc->realize = f1c100s_gpio_realize;
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
    printf("call %s()\n", __func__);

    type_register_static(&f1c100s_info);
}

type_init(f1c100s_register)
