#include <kernel/exception.h>

#define ENABLE_INTERRUPT() (asm volatile("msr daifclr, 0xf"));
#define DISABLE_INTERRUPT() (asm volatile("msr daifset, 0xf"));

void sync_el0_64_handler()
{
    unsigned long spsr_el1;
    unsigned long elr_el1;
    unsigned long esr_el1;

    asm volatile("mrs %0, SPSR_EL1\n"
                 : "=r"(spsr_el1)
                 :
                 : "memory");
    asm volatile("mrs %0, ELR_EL1\n"
                 : "=r"(elr_el1)
                 :
                 : "memory");
    asm volatile("mrs %0, ESR_EL1\n"
                 : "=r"(esr_el1)
                 :
                 : "memory");

    uart_printf("+-------------------------------+\n");
    uart_printf("| SPSR_EL1:\t0x%x\t|\n| ESR_EL1:\t0x%x\t|\n| ELR_EL1:\t0x%x\t|\n", spsr_el1, esr_el1, elr_el1);
    uart_printf("+-------------------------------+\n");
}

void irq_handler()
{
    if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // Timer interrupt
    {
        timer_pop();
    }

    if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // GPU interrupt
    {
        if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT) // UART interrupt
        {
            uart_interrupt_handler();
        }
    }
}

void invalid_exception_handler(unsigned long long x0)
{
    uart_printf("invalid ecception : 0x%x\n", x0);
}