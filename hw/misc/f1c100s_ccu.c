#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/f1c100s_log.h"
#include "migration/vmstate.h"
#include "hw/sysbus.h"
#include "hw/misc/f1c100s_ccu.h"
 
static uint64_t f1c100s_ccu_read(void *opaque, hwaddr offset, unsigned size)
{
    f1c100s_ccu_state *s = F1C100S_CCU(opaque);

    trace("call %s(offset=0x%lx, size=%d)\n", __func__, offset, size);
 
    switch (offset) {
    case PLL_CPU_CTRL_REG:
        trace("%s, PLL_CPU_CTRL_REG=0x%x\n", __func__, s->reg[PLL_CPU_CTRL_REG]);
        return s->reg[PLL_CPU_CTRL_REG];
    }
    return 0;
}
 
static void f1c100s_ccu_write(void *opaque, hwaddr offset, uint64_t val, unsigned size)
{
    f1c100s_ccu_state *s = F1C100S_CCU(opaque);

    trace("call %s(offset=0x%lx, val=0x%lx, size=%d)\n", __func__, offset, val, size);

    switch (offset) {
    case PLL_CPU_CTRL_REG:
        s->reg[PLL_CPU_CTRL_REG] = val;
        s->reg[PLL_CPU_CTRL_REG] |= LOCK;
        break;
    }
}
 
static const MemoryRegionOps f1c100s_ccu_ops = {
    .read = f1c100s_ccu_read,
    .write = f1c100s_ccu_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};
 
static void f1c100s_ccu_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    f1c100s_ccu_state *s = F1C100S_CCU(dev);
 
    trace("call %s()\n", __func__);
 
    memory_region_init_io(&s->iomem, OBJECT(s), &f1c100s_ccu_ops, s, TYPE_F1C100S_CCU, 0x400);
    sysbus_init_mmio(sbd, &s->iomem);
}
 
static void f1c100s_ccu_init(Object *obj)
{
    trace("call %s()\n", __func__);
}
 
static void f1c100s_ccu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
 
    trace("call %s()\n", __func__);
    dc->realize = f1c100s_ccu_realize;
}
 
static const TypeInfo f1c100s_ccu_info = {
    .name = TYPE_F1C100S_CCU,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = f1c100s_ccu_init,
    .instance_size = sizeof(f1c100s_ccu_state),
    .class_init = f1c100s_ccu_class_init,
};
 
static void f1c100s_ccu_register(void)
{
    trace("call %s()\n", __func__);
 
    type_register_static(&f1c100s_ccu_info);
}
 
type_init(f1c100s_ccu_register)
