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

#ifndef __INTC_F1C100S__
#define __INTC_F1C100S__

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_AW_F1C100S_INTC  "f1c100s-intc"
OBJECT_DECLARE_SIMPLE_TYPE(AwF1c100sIntcState, AW_F1C100S_INTC)

struct AwF1c100sIntcState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    qemu_irq parent_irq;

    uint32_t vector;
    uint32_t base_addr;
    uint32_t nmi_ctl;
    uint32_t pending[2];
    uint32_t enable[2];
    uint32_t mask[2];
};

#endif
