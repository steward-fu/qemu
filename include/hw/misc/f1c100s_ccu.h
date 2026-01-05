#ifndef __CCU_F1C100S_H__
#define __CCU_F1C100S_H__
 
#include "qom/object.h"
#include "hw/sysbus.h"
 
#define TYPE_F1C100S_CCU "f1c100s-ccu"
OBJECT_DECLARE_SIMPLE_TYPE(f1c100s_ccu_state, F1C100S_CCU)

#define LOCK                (1 << 28)
#define PLL_CPU_CTRL_REG    0x0000
#define CPU_CLK_SRC_REG     0x0050
 
struct f1c100s_ccu_state {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

    uint32_t reg[1024];
};
 
#endif
