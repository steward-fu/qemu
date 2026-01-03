/*
 * Copyright (C) 2025 Steward <steward.fu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef __GPIO_F1C100S_H__
#define __GPIO_F1C100S_H__

#include "qom/object.h"
#include "hw/sysbus.h"

#define PE_CFG0     ((4 * 0x24) + 0x00)
#define PE_CFG1     ((4 * 0x24) + 0x04)
#define PE_CFG2     ((4 * 0x24) + 0x08)
#define PE_CFG3     ((4 * 0x24) + 0x0c)
#define PE_DATA     ((4 * 0x24) + 0x10)

struct f1c100s_gpio_t {
    uint32_t dir;
    uint32_t data;
};

#define TYPE_AW_F1C100S_GPIO "f1c100s-gpio"
OBJECT_DECLARE_SIMPLE_TYPE(AwF1c100sGpioState, AW_F1C100S_GPIO)

struct AwF1c100sGpioState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
};

#endif
