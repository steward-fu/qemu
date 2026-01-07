#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/f1c100s_log.h"
#include "migration/vmstate.h"
#include "hw/sysbus.h"
#include "hw/intc/f1c100s.h"
#include "hw/irq.h"

static void f1c100s_intc_update(f1c100s_intc_state *s)
{
    bool active = (s->enable & s->pending) != 0;

    trace("call %s(active=%d)\n", __func__, active);

    qemu_set_irq(s->parent_irq, active);
}

static uint64_t f1c100s_intc_read(void *opaque, hwaddr offset, unsigned size)
{
    f1c100s_intc_state *s = opaque;

    trace("call %s(offset=0x%lx, size=%d)\n", __func__, offset, size);

    switch (offset) {
    case INTC_EN_REG0:
        return s->enable;
    case INTC_PEND_REG0:
        return s->pending;
    }

    return 0;
}

static void f1c100s_intc_write(void *opaque, hwaddr offset, uint64_t val, unsigned size)
{
    int i = 0;
    f1c100s_intc_state *s = opaque;

    trace("call %s(offset=0x%lx, val=0x%lx, size=%d)\n", __func__, offset, val, size);

    switch (offset) {
    case INTC_FF_REG0:
        for (i = 0; i < 64; i++) {
            if ((1ULL << i) & s->enable & val) {
                qemu_set_irq(qdev_get_gpio_in(DEVICE(s), i), 1);
            }
        }
        s->pending |= val;
        break;
    case INTC_EN_REG0:
        s->enable = val;
        break;
    case INTC_PEND_REG0:
        s->pending &= ~val;
        break;
    }

    f1c100s_intc_update(s);
}

static const MemoryRegionOps f1c100s_intc_ops = {
    .read = f1c100s_intc_read,
    .write = f1c100s_intc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void f1c100s_intc_set_irq(void *opaque, int irq, int level)
{
    f1c100s_intc_state *s = opaque;

    trace("call %s(irq=%d, level=%d)\n", __func__, irq, level);

    if (level) {
        s->pending |= (1ULL << irq);
        f1c100s_intc_update(s);
    }
}

static void f1c100s_intc_reset(DeviceState *dev)
{
    f1c100s_intc_state *s = F1C100S_INTC(dev);

    trace("call %s()\n", __func__);

    s->pending = 0;
    s->enable  = 0;
    qemu_set_irq(s->parent_irq, 0);
}

static void f1c100s_intc_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    f1c100s_intc_state *s = F1C100S_INTC(dev);

    trace("call %s()\n", __func__);

    memory_region_init_io(&s->iomem, OBJECT(s), &f1c100s_intc_ops, s, TYPE_F1C100S_INTC, 0x400);
    sysbus_init_mmio(sbd, &s->iomem);

    sysbus_init_irq(sbd, &s->parent_irq);
    qdev_init_gpio_in(dev, f1c100s_intc_set_irq, 41);
}

static void f1c100s_intc_init(Object *obj)
{
    trace("call %s()\n", __func__);
}

static void f1c100s_intc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    trace("call %s()\n", __func__);
    dc->reset = f1c100s_intc_reset;
    dc->realize = f1c100s_intc_realize;
}

static const TypeInfo f1c100s_intc_info = {
    .name = TYPE_F1C100S_INTC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = f1c100s_intc_init,
    .instance_size = sizeof(f1c100s_intc_state),
    .class_init = f1c100s_intc_class_init,
};

static void f1c100s_intc_register(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&f1c100s_intc_info);
}

type_init(f1c100s_intc_register)
