#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <kernel/list.h>
#include <drivers/uart.h>

#define CRITICAL_SECTION_START      critical_section_start()
#define CRITICAL_SECTION_END        critical_section_end()

#define CORE0_INTERRUPT_SOURCE      ((volatile unsigned int *)PHYS_TO_VIRT((0x40000060)))

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

/* Exception class type */
#define SVC_INS_EXEC 0b010101
#define DATA_ABORT_LOWER 0b100100
#define INS_ABORT_LOWER 0b100000

/* Page fault type */
#define TRANS_FAULT_LV0 0b000100 
#define TRANS_FAULT_LV1 0b000101 
#define TRANS_FAULT_LV2 0b000110 
#define TRANS_FAULT_LV3 0b000111 

typedef struct exception {
    uint32_t iss : 25, // Instruction specific syndrome
             il  : 1,  // Instruction length bit
             ec  : 6;  // Exception class
} exception_t;

exception_t get_current_exception();

typedef void (*irq_callback)();

typedef struct irq_t {
    list_head_t listhead;
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

void critical_section_start();
void critical_section_end();

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
    return (ret >> 2);
}

#endif