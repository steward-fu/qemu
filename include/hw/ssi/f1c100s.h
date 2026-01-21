#ifndef __SPI_F1C100S_H__
#define __SPI_F1C100S_H__

#include "qemu/fifo8.h"
#include "qom/object.h"
#include "hw/sysbus.h"

#define TYPE_F1C100S_SPI "f1c100s-spi"
OBJECT_DECLARE_SIMPLE_TYPE(f1c100s_spi_state, F1C100S_SPI)

#define GCR  0x04
#define TCR  0x08
#define ISR  0x14
#define FCR  0x18
#define FSR  0x1c
#define WCR  0x20
#define CCR  0x24
#define MBC  0x30
#define MTC  0x34
#define BCC  0x38
#define TXD  0x200
#define RXD  0x300

struct f1c100s_spi_state {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    SSIBus *spi;
    qemu_irq ss_line;

    Fifo8 tx_fifo;
    Fifo8 rx_fifo;
    uint8_t fifo_depth;
    uint8_t burst_len;
};

#endif
