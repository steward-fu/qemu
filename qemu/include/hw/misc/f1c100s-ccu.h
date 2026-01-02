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

#ifndef __MISC_F1C100S_CCU_H__
#define __MISC_F1C100S_CCU_H__

#include "qom/object.h"
#include "hw/sysbus.h"

#define TYPE_AW_F1C100S_CCU "f1c100s-ccu"
OBJECT_DECLARE_SIMPLE_TYPE(AwF1c100sClockCtlState, AW_F1C100S_CCU)

struct AwF1c100sClockCtlState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
};

#endif
