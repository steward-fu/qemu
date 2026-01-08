#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/f1c100s_log.h"
#include "migration/vmstate.h"
#include "hw/char/f1c100s_uart.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "trace.h"

static uint64_t f1c100s_uart_read(void *opaque, hwaddr addr, unsigned size)
{
    trace("call %s()\n", __func__);

    switch (addr) {
    case USR:
        return (1ULL << 1);
    }
    return 0;
}

static void f1c100s_uart_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    f1c100s_uart_state *s = F1C100S_UART(opaque);

    trace("call %s(addr=0x%lx, val=0x%lx, size=%d)\n", __func__, addr, val, size);

    switch (addr) {
    case LCR:
        if ((val & (1ULL << 7)) == 0) {
            s->cnt = 0;
            memset(s->buf, 0, sizeof(s->buf));
        }
        break;
    case RBR:
        s->buf[s->cnt++] = (char)val;
        s->cnt %= sizeof(s->buf);

        if (val == 0) {
            printf("%s\n", s->buf);
        }
        break;
    }
}

static const MemoryRegionOps f1c100s_uart_ops = {
    .read = f1c100s_uart_read,
    .write = f1c100s_uart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void f1c100s_uart_reset(DeviceState *dev)
{
    trace("call %s()\n", __func__);
}

static void f1c100s_uart_realize(DeviceState *dev, Error **errp)
{
    trace("call %s()\n", __func__);
}

static void f1c100s_uart_init(Object *obj)
{
    f1c100s_uart_state *s = F1C100S_UART(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    trace("call %s()\n", __func__);

    memory_region_init_io(&s->iomem, obj, &f1c100s_uart_ops, s, TYPE_F1C100S_UART, 0x400);
    sysbus_init_mmio(sbd, &s->iomem);
}

static Property f1c100s_uart_properties[] = {
    DEFINE_PROP_CHR("chardev", f1c100s_uart_state, chr),
    DEFINE_PROP_END_OF_LIST()
};

static void f1c100s_uart_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    trace("call %s()\n", __func__);

    dc->reset = f1c100s_uart_reset;
    dc->realize = f1c100s_uart_realize;
    device_class_set_props(dc, f1c100s_uart_properties);
}

static const TypeInfo f1c100s_uart_info = {
    .name = TYPE_F1C100S_UART,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(f1c100s_uart_state),
    .instance_init = f1c100s_uart_init,
    .class_init = f1c100s_uart_class_init
};

static void f1c100s_uart_register_types(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&f1c100s_uart_info);
}

type_init(f1c100s_uart_register_types)
