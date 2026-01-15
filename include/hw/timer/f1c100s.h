#ifndef __TIMER_F1C100S_H__
#define __TIMER_F1C100S_H__

#include "qom/object.h"
#include "hw/sysbus.h"
#include "hw/ptimer.h"

#define TYPE_F1C100S_TIMER "f1c100s-timer"
OBJECT_DECLARE_SIMPLE_TYPE(f1c100s_timer_state, F1C100S_TIMER)

#define MAX_TIMER           3
#define TIMER0              0x00
#define TIMER1              0x01
#define TIMER2              0x02

#define TMR_IRQ_EN_REG      0x00
#define TMR_IRQ_STA_REG     0x04
#define TMR0_CTRL_REG       0x10
#define TMR0_INTV_VALUE_REG 0x14
#define TMR0_CUR_VALUE_REG  0x18

struct f1c100s_timer_state {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

    qemu_irq irq[MAX_TIMER];
    ptimer_state *ptimer[MAX_TIMER];

    int pres[MAX_TIMER];
    int count[MAX_TIMER];
    bool start[MAX_TIMER];
    bool enable_irq[MAX_TIMER];
    bool enable_timer[MAX_TIMER];
    bool irq_pending[MAX_TIMER];
};

#endif
