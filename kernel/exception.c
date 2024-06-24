#include <kernel/exception.h>

LIST_HEAD(irq_list);

int curr_irq_prio = 32768;

void irq_add(irq_callback callback, int prio)
{
    irq_t *irq = malloc(sizeof(irq_t));

    // init listhead
    INIT_LIST_HEAD(&irq->listhead);    

    // set callback & prio
    irq->callback = callback;
    irq->prio = prio;

    struct list_head *it = NULL;

    disable_interrupt(); // critical section start
    list_for_each(it, &irq_list)
    {
        if (((irq_t *)it)->prio > irq->prio)
        {
            list_add(&irq->listhead, it->prev);
            break;
        }
    }

    // highest priority
    if (list_is_head(it, &irq_list))
        list_add_tail(&irq->listhead, &irq_list);
    enable_interrupt(); // critical section end
}

void irq_pop()
{
    while (!list_empty(&irq_list))
    {
        irq_t *next_irq = (irq_t *)irq_list.next;

        // Current irq has higher prio
        if (curr_irq_prio <= next_irq->prio)
            return;

        // New irq has higher prio
        disable_interrupt(); // critical section start
        list_del_entry(irq_list.next);
        enable_interrupt(); // critical section end

        int prev_irq_prio = curr_irq_prio;
        curr_irq_prio = next_irq->prio;

        next_irq->callback();

        curr_irq_prio = prev_irq_prio;
    }
}

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
        timer_interrupt_disable();
        irq_add(timer_pop, 0);
        irq_pop();
    }

    if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // GPU interrupt
    {
        if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT) // UART interrupt
        {
            uart_disable_tx_interrupt();
            uart_disable_rx_interrupt();
            irq_add(uart_interrupt_handler, 1);
            irq_pop();
        }
    }
}

void invalid_exception_handler(unsigned long long x0)
{
    uart_printf("invalid exception : 0x%x\n", x0);
}