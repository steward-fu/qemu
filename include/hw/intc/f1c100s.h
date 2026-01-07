#ifndef __INTC_F1C100S_H__
#define __INTC_F1C100S_H__

#include "qom/object.h"
#include "hw/sysbus.h"

#define TYPE_F1C100S_INTC "f1c100s-intc"
OBJECT_DECLARE_SIMPLE_TYPE(f1c100s_intc_state, F1C100S_INTC)

#define INTC_BASE_ADDR_REG  0x04
#define INTC_PEND_REG0      0x10
#define INTC_PEND_REG1      0x14
#define INTC_EN_REG0        0x20
#define INTC_EN_REG1        0x24
#define INTC_MASK_REG0      0x30
#define INTC_MASK_REG1      0x34
#define INTC_RESP_REG0      0x40
#define INTC_RESP_REG1      0x44
#define INTC_FF_REG0        0x50
#define INTC_FF_REG1        0x54

struct f1c100s_intc_state {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    qemu_irq parent_irq;

    uint64_t pending;
    uint64_t enable;
};

#endif
