#ifndef __GPIO_F1C100S_H__
#define __GPIO_F1C100S_H__

#include "qom/object.h"
#include "hw/sysbus.h"

#define TYPE_F1C100S_GPIO "f1c100s-gpio"
OBJECT_DECLARE_SIMPLE_TYPE(f1c100s_gpio_state, F1C100S_GPIO)

struct f1c100s_gpio_state {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
};

#endif
