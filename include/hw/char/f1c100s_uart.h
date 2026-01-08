#ifndef __CHAR_UART_F1C100S_H__
#define __CHAR_UART_F1C100S_H__

#include "hw/sysbus.h"
#include "chardev/char-fe.h"
#include "qom/object.h"
#include "qemu/fifo8.h"

#define TYPE_F1C100S_UART "f1c100s-uart"
OBJECT_DECLARE_SIMPLE_TYPE(f1c100s_uart_state, F1C100S_UART)

#define RBR 0x0000
#define DLL 0x0000
#define DLH 0x0004
#define IER 0x0004
#define IIR 0x0008
#define LCR 0x000c
#define MCR 0x0010
#define USR 0x007c

struct f1c100s_uart_state {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    CharBackend chr;

    int cnt;
    char buf[255];
};

#endif
