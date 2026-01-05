#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/f1c100s_log.h"
#include "migration/vmstate.h"
#include "hw/sysbus.h"
#include "hw/gpio/f1c100s.h"

static uint64_t f1c100s_gpio_read(void *opaque, hwaddr offset, unsigned size)
{
    trace("call %s(offset=0x%lx, size=%d)\n", __func__, offset, size);

    return 0;
}

static void f1c100s_gpio_write(void *opaque, hwaddr offset, uint64_t val, unsigned size)
{
    trace("call %s(offset=0x%lx, val=0x%lx, size=%d)\n", __func__, offset, val, size);
}

static const MemoryRegionOps f1c100s_gpio_ops = {
    .read = f1c100s_gpio_read,
    .write = f1c100s_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void f1c100s_gpio_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    f1c100s_gpio_state *s = F1C100S_GPIO(dev);

    trace("call %s()\n", __func__);

    memory_region_init_io(&s->iomem, OBJECT(s), &f1c100s_gpio_ops, s, TYPE_F1C100S_GPIO, 0x400);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void f1c100s_gpio_init(Object *obj)
{
    trace("call %s()\n", __func__);
}

static void f1c100s_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    trace("call %s()\n", __func__);
    dc->realize = f1c100s_gpio_realize;
}

static const TypeInfo f1c100s_gpio_info = {
    .name = TYPE_F1C100S_GPIO,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = f1c100s_gpio_init,
    .instance_size = sizeof(f1c100s_gpio_state),
    .class_init = f1c100s_gpio_class_init,
};

static void f1c100s_gpio_register(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&f1c100s_gpio_info);
}

type_init(f1c100s_gpio_register)
