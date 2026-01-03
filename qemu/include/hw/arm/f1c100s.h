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

#ifndef __ARM_F1C100S_H__
#define __ARM_F1C100S_H__

#include "hw/intc/f1c100s.h"
#include "hw/gpio/f1c100s.h"
#include "hw/misc/f1c100s-ccu.h"
#include "hw/misc/allwinner-sid.h"
#include "hw/sd/allwinner-sdhost.h"
#include "hw/timer/allwinner-a10-pit.h"
#include "hw/ssi/allwinner-sun6i-spi.h"
#include "hw/i2c/allwinner-i2c.h"
#include "hw/char/f1c100s-uart.h"

#include "target/arm/cpu.h"
#include "qom/object.h"

enum {
    AW_F1C100S_DEV_SRAM_A1,
    AW_F1C100S_DEV_SPI0,
    AW_F1C100S_DEV_SPI1,
    AW_F1C100S_DEV_CCU,
    AW_F1C100S_DEV_INTC,
    AW_F1C100S_DEV_GPIO,
    AW_F1C100S_DEV_TIMER,
    AW_F1C100S_DEV_UART0,
    AW_F1C100S_DEV_UART1,
    AW_F1C100S_DEV_UART2,
    AW_F1C100S_DEV_SID,
    AW_F1C100S_DEV_MMC0,
    AW_F1C100S_DEV_MMC1,
    AW_F1C100S_DEV_TWI0,
    AW_F1C100S_DEV_TWI1,
    AW_F1C100S_DEV_TWI2,
    AW_F1C100S_DEV_LOG_BUF,
    AW_F1C100S_DEV_SDRAM,
    AW_F1C100S_DEV_BOOTROM,
};

#define TYPE_AW_F1C100S "f1c100s"
OBJECT_DECLARE_SIMPLE_TYPE(AwF1c100sState, AW_F1C100S)

struct AwF1c100sState {
    DeviceState parent_obj;
    Notifier shutdown_notifier;
    const hwaddr *memmap;

    ARMCPU cpu;
    AwF1c100sClockCtlState ccu;
    AwF1c100sIntcState intc;
    AwF1c100sGpioState gpio;
    AwA10PITState timer;
    AwSun6iSpiState spi[2];
    AWI2CState i2c[3];
    AwSidState sid;
    AwSdHostState mmc[2];
    AwUartState uart[3];
    MemoryRegion sram_a1;
    MemoryRegion sram_logbuf;
    MemoryRegion bootrom;
};

#endif
