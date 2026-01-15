#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/f1c100s_log.h"
#include "migration/vmstate.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/timer/f1c100s.h"

static void timer0_handler(void *opaque)
{
    f1c100s_timer_state *s = F1C100S_TIMER(opaque);

    trace("call %s\n", __func__);
    if (s->enable_irq[TIMER0]) {
        if (s->irq_pending[TIMER0] == false) {
            s->irq_pending[TIMER0] = true;
            qemu_set_irq(s->irq[TIMER0], 1);
            trace("set timer0 irq = 1\n");
        }
    }
}

static uint64_t f1c100s_timer_read(void *opaque, hwaddr offset, unsigned size)
{
    trace("call %s(offset=0x%lx, size=%d)\n", __func__, offset, size);

    return 0;
}

static void f1c100s_timer_write(void *opaque, hwaddr offset, uint64_t val, unsigned size)
{
    int i = 0;
    bool oneshot = true;
    f1c100s_timer_state *s = F1C100S_TIMER(opaque);

    trace("call %s(offset=0x%lx, val=0x%lx, size=%d)\n", __func__, offset, val, size);

    switch (offset) {
    case TMR0_INTV_VALUE_REG:
        s->count[0] = val;
        break;
    case TMR0_CTRL_REG:
        ptimer_transaction_begin(s->ptimer[0]);

        if (val & (1ULL << 2)) {
            ptimer_set_freq(s->ptimer[0], 24000000);
            trace("timer0 clock = 24MHz\n");
        }
        else {
            ptimer_set_freq(s->ptimer[0], 32000);
            trace("timer0 clock = 32KHz\n");
        }

        s->pres[0] = 1UL << ((val >> 4) & 7);
        trace("timer0 pre-scale = %d\n", s->pres[0]);

        s->enable_timer[0] = (val & 1);
        trace("timer0 enable = %d\n", s->enable_timer[0]);

        oneshot = !!(val & (1ULL << 7));
        ptimer_set_count(s->ptimer[0], s->count[0] * s->pres[0]);
        ptimer_set_limit(s->ptimer[0], s->count[0] * s->pres[0], 1);
        trace("timer0 count = %d, oneshot = %d\n", s->count[0] * s->pres[0], oneshot);

        if (s->enable_timer[i]) {
            if (s->start[i] == false) {
                s->start[i] = true;

                ptimer_run(s->ptimer[i], oneshot);
                trace("timer0 run = 1\n");
            }
        }
        else {
            if (s->start[i] == true) {
                s->start[i] = false;
                ptimer_stop(s->ptimer[i]);
                trace("timer0 stop = 1\n");
            }
        }

        ptimer_transaction_commit(s->ptimer[0]);
        break;
    case TMR_IRQ_EN_REG:
        for (i = 0; i <MAX_TIMER; i++) {
            s->enable_irq[i] = !!(val & (1ULL << i));
            trace("timer%d enable = %d\n", i, s->enable_irq[i]);
        }
        break;
    case TMR_IRQ_STA_REG:
        for (i = 0; i < MAX_TIMER; i++) {
            if ((val & (1 << i)) == 0) {
                continue;
            }

            if (s->irq_pending[i]) {
                s->irq_pending[i] = false;
                qemu_set_irq(s->irq[i], 0);
                trace("set timer%d irq = 0\n", i);
            }
        }
        break;
    }
}

static const MemoryRegionOps f1c100s_timer_ops = {
    .read = f1c100s_timer_read,
    .write = f1c100s_timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void f1c100s_timer_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    f1c100s_timer_state *s = F1C100S_TIMER(dev);

    trace("call %s()\n", __func__);

    memory_region_init_io(&s->iomem, OBJECT(s), &f1c100s_timer_ops, s, TYPE_F1C100S_TIMER, 0x400);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq[0]);
    sysbus_init_irq(sbd, &s->irq[1]);
    sysbus_init_irq(sbd, &s->irq[2]);

    s->ptimer[0] = ptimer_init(timer0_handler, s, 0);
    ptimer_transaction_begin(s->ptimer[0]);
    ptimer_set_freq(s->ptimer[0], 24000000);
    ptimer_transaction_commit(s->ptimer[0]);
}

static void f1c100s_timer_init(Object *obj)
{
    trace("call %s()\n", __func__);
}

static void f1c100s_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    trace("call %s()\n", __func__);
    dc->realize = f1c100s_timer_realize;
}

static const TypeInfo f1c100s_timer_info = {
    .name = TYPE_F1C100S_TIMER,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = f1c100s_timer_init,
    .instance_size = sizeof(f1c100s_timer_state),
    .class_init = f1c100s_timer_class_init,
};

static void f1c100s_timer_register(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&f1c100s_timer_info);
}

type_init(f1c100s_timer_register)
