#ifndef __GPIO_F1C100S_H__
#define __GPIO_F1C100S_H__

#include "qom/object.h"
#include "hw/sysbus.h"

#define TYPE_AW_F1C100S_GPIO "f1c100s-gpio"
OBJECT_DECLARE_SIMPLE_TYPE(AwF1c100sGpioState, AW_F1C100S_GPIO)

struct AwF1c100sGpioState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
};

#endif
