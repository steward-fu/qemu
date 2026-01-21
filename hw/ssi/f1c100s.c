#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/f1c100s_log.h"
#include "migration/vmstate.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/ssi/ssi.h"
#include "hw/ssi/f1c100s.h"

static uint64_t f1c100s_spi_read(void *opaque, hwaddr offset, unsigned size)
{
    f1c100s_spi_state *s = F1C100S_SPI(opaque);

    trace("call %s(offset=0x%lx, size=%d)\n", __func__, offset, size);

    switch (offset) {
    case RXD:
        while (!fifo8_is_empty(&s->rx_fifo)) {
            trace("spi rx data = 0x%x\n", fifo8_pop(&s->rx_fifo));
        }
        break;
    }

    return 0;
}

static void f1c100s_spi_write(void *opaque, hwaddr offset, uint64_t val, unsigned size)
{
    int i = 0;
    uint8_t tx = 0;
    uint8_t rx = 0;
    f1c100s_spi_state *s = F1C100S_SPI(opaque);

    trace("call %s(offset=0x%lx, val=0x%lx, size=%d)\n", __func__, offset, val, size);

    switch (offset) {
    case MBC:
        s->burst_len = val;
        trace("spi brust size = %d\n", s->burst_len);
        break;
    case TXD:
        if (!fifo8_is_full(&s->tx_fifo)) {
            fifo8_push(&s->tx_fifo, (uint8_t)val);
        }
        break;
    case TCR:
        if (val & (1ULL << 31)) {
            qemu_irq_lower(s->ss_line);
            for (i = 0; i < s->burst_len; i++) {
                tx = 0xff;
                if (!fifo8_is_empty(&s->tx_fifo)) {
                    tx = fifo8_pop(&s->tx_fifo);
                }
                trace("spi tx data = 0x%x\n", tx);
                rx = ssi_transfer(s->spi, tx);

                if (!fifo8_is_full(&s->rx_fifo)) {
                    fifo8_push(&s->rx_fifo, rx);
                }
            }
            qemu_irq_raise(s->ss_line);
        }
        break;
    }
}

static const MemoryRegionOps f1c100s_spi_ops = {
    .read = f1c100s_spi_read,
    .write = f1c100s_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void f1c100s_spi_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    f1c100s_spi_state *s = F1C100S_SPI(dev);

    trace("call %s()\n", __func__);

    sysbus_init_irq(sbd, &s->ss_line);
    memory_region_init_io(&s->iomem, OBJECT(s), &f1c100s_spi_ops, s, TYPE_F1C100S_SPI, 4 * KiB);
    sysbus_init_mmio(sbd, &s->iomem);
    s->spi = ssi_create_bus(dev, "ssi");
    fifo8_create(&s->tx_fifo, s->fifo_depth);
    fifo8_create(&s->rx_fifo, s->fifo_depth);
}

static void f1c100s_spi_init(Object *obj)
{
    trace("call %s()\n", __func__);
}

static Property f1c100s_spi_properties[] = {
    DEFINE_PROP_UINT8("fifo_depth", f1c100s_spi_state, fifo_depth, 64),
    DEFINE_PROP_END_OF_LIST()
};

static void f1c100s_spi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    trace("call %s()\n", __func__);
    dc->realize = f1c100s_spi_realize;
    device_class_set_props(dc, f1c100s_spi_properties);
}

static const TypeInfo f1c100s_spi_info = {
    .name = TYPE_F1C100S_SPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = f1c100s_spi_init,
    .instance_size = sizeof(f1c100s_spi_state),
    .class_init = f1c100s_spi_class_init,
};

static void f1c100s_spi_register(void)
{
    trace("call %s()\n", __func__);

    type_register_static(&f1c100s_spi_info);
}

type_init(f1c100s_spi_register)
