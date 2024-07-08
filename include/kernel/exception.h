#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <drivers/uart.h>
#include <drivers/mmio.h>
#include <kernel/timer.h>
#include <kernel/list.h>
#include <kernel/syscall.h>
#include <kernel/trapframe.h>
#include <kernel/sched.h>
#include <mm/mm.h>
#include <stddef.h>

#define CRITICAL_SECTION_START      critical_section_start()
#define CRITICAL_SECTION_END        critical_section_end()

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

typedef void (*irq_callback)();

static int lock = 0;

typedef struct irq_t {
    struct list_head listhead;
    irq_callback callback;
    int prio;                   // 0 is the highest priority
} irq_t;

static inline void enable_interrupt()
{
    asm volatile("msr daifclr, #0b1111");
}

static inline void disable_interrupt()
{
    asm volatile("msr daifset, #0b1111");
}

static inline void critical_section_end()
{
    --lock;
    if (lock == 0)
        enable_interrupt();
}

static inline void critical_section_start()
{
    ++lock;
    disable_interrupt();
}

static inline uint64_t interrupt_status()
{
    uint64_t ret;
    asm volatile("mrs   %0, daif" : "=r"(ret));
    return ret;
}

static inline uint64_t current_exception_level()
{
    uint64_t ret;
    asm volatile("mrs   %0, CurrentEL" : "=r"(ret));
    return ret;
}

#endif