#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <drivers/uart.h>
#include <drivers/mmio.h>
#include <kernel/timer.h>

#define CORE0_INTERRUPT_SOURCE      ((volatile unsigned int *)(0x40000060))

#define IRQ_BASIC_PENDING           ((volatile unsigned int *)(MMIO_BASE + 0x0000B200))
#define IRQ_PENDING_1               ((volatile unsigned int *)(MMIO_BASE + 0x0000B204))
#define IRQ_PENDING_2               ((volatile unsigned int *)(MMIO_BASE + 0x0000B208))
#define FIQ_CONTROL                 ((volatile unsigned int *)(MMIO_BASE + 0x0000B20C))
#define ENABLE_IRQS_1               ((volatile unsigned int *)(MMIO_BASE + 0x0000B210))
#define ENABLE_IRQS_2               ((volatile unsigned int *)(MMIO_BASE + 0x0000B214))
#define ENABLE_BASIC_IRQS           ((volatile unsigned int *)(MMIO_BASE + 0x0000B218))
#define DISABLE_IRQS_1              ((volatile unsigned int *)(MMIO_BASE + 0x0000B21C))
#define DISABLE_IRQS_2              ((volatile unsigned int *)(MMIO_BASE + 0x0000B220))
#define DISABLE_BASIC_IRQS          ((volatile unsigned int *)(MMIO_BASE + 0x0000B224))

#define IRQ_PENDING_1_AUX_INT       (1 << 29)
#define INTERRUPT_SOURCE_GPU        (1 << 8)
#define INTERRUPT_SOURCE_CNTPNSIRQ  (1 << 1)

static inline void enable_interrupt()
{
    asm volatile("msr daifclr, #0b1111");
}

static inline void disable_interrupt()
{
    asm volatile("msr daifset, #0b1111");
}

#endif