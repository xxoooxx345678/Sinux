#include "uart.h"
#include "exception.h"
#include "timer.h"
#include "task.h"

void sync_el0_64_handler(unsigned long long x0){
    unsigned long spsr_el1;
    unsigned long elr_el1;
    unsigned long esr_el1;

    asm volatile("mrs %0, SPSR_EL1\n" : "=r" (spsr_el1) :  : "memory");
    asm volatile("mrs %0, ELR_EL1\n" : "=r" (elr_el1) :  : "memory");
    asm volatile("mrs %0, ESR_EL1\n" : "=r" (esr_el1) :  : "memory");

    uart_async_printf("+-------------------------------+\r\n");
    uart_async_printf("| SPSR_EL1:\t0x%x\t|\r\n| ESR_EL1:\t0x%x\t|\r\n| ELR_EL1:\t0x%x\t|\r\n", spsr_el1, esr_el1, elr_el1);
    uart_async_printf("+-------------------------------+\r\n");
}

void irq_handler(unsigned long long x0){
    if(*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception  
    {
        /*
        AUX_MU_IIR (interrupt status)
        bits[2:1] :
        00 : No interrupts
        01 : Transmit holding register empty
        10 : Receiver holds valid byte
        11: <Not possible> 
        */

        if (*AUX_MU_IIR & 0b10) // writable
        {
            disable_mini_uart_w_interrupt();
            add_task(uart_interrupt_w_handler, 1);
            pop_task();
        }
        else if (*AUX_MU_IIR & 0b100) // readable
        {
            disable_mini_uart_r_interrupt();
            add_task(uart_interrupt_r_handler, 1);
            pop_task();
        }
        else
            uart_printf("uart handler error\r\n");

    }
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) 
    {
        core_timer_disable();   
        add_task(core_timer_handler, -1);
        pop_task();
        core_timer_enable();    
    }
}

void error_exception_handler(unsigned long long x0){
    uart_printf("invalid exception : 0x%x\r\n",x0);
    uart_async_getc();
}

void enable_interrupt(){
    asm volatile("msr daifclr, 0xf");
}

void disable_interrupt(){
    asm volatile("msr daifset, 0xf");
}