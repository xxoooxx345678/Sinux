#include <kernel/exception.h>

extern thread_t *cur_thread;

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
    CRITICAL_SECTION_START; // critical section start

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
    CRITICAL_SECTION_END; // critical section end
}

void irq_pop()
{
    while (!list_empty(&irq_list))
    {
        CRITICAL_SECTION_START;
        irq_t *next_irq = (irq_t *)irq_list.next;

        // Current irq has higher prio
        if (curr_irq_prio <= next_irq->prio)
        {
            CRITICAL_SECTION_END;
            return;
        }

        // New irq has higher prio
        list_del_entry(irq_list.next);

        int prev_irq_prio = curr_irq_prio;
        curr_irq_prio = next_irq->prio;
        CRITICAL_SECTION_END;

        next_irq->callback();

        CRITICAL_SECTION_START;
        curr_irq_prio = prev_irq_prio;
        free(next_irq);
        CRITICAL_SECTION_END;
    }
}

void sync_el0_64_handler(trapframe_t *tf)
{
    enable_interrupt();
    cur_thread->trapframe = tf;

    uint64_t syscall_no = cur_thread->trapframe->x8;

    switch (syscall_no)
    {
    case 0:
        cur_thread->trapframe->x0 = sys_getpid();
        break;
    case 1:
        cur_thread->trapframe->x0 = sys_uartread((char *)cur_thread->trapframe->x0, (size_t)cur_thread->trapframe->x1);
        break;
    case 2:
        cur_thread->trapframe->x0 = sys_uartwrite((char *)cur_thread->trapframe->x0, (size_t)cur_thread->trapframe->x1);
        break;
    case 3:
        cur_thread->trapframe->x0 = sys_exec((char *)cur_thread->trapframe->x0, (char **)cur_thread->trapframe->x1);
        break;
    case 4:
        cur_thread->trapframe->x0 = sys_fork();
        break;
    case 5:
        sys_exit();
        break;
    case 6:
        cur_thread->trapframe->x0 = sys_mbox_call((unsigned char)cur_thread->trapframe->x0, (unsigned int *)cur_thread->trapframe->x1);
        break;
    case 7:
        sys_kill((int)cur_thread->trapframe->x0);
        break;
    case 8:
        sys_signal((int)cur_thread->trapframe->x0, (void (*)(int))cur_thread->trapframe->x1);
        break;
    case 9:
        sys_sigkill((int)cur_thread->trapframe->x0, (int)cur_thread->trapframe->x1);
        break;
    case 30:
        sys_sigreturn();
        break;
    }
}

void irq_handler()
{
    if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // Timer interrupt
    {
        timer_interrupt_disable();
        irq_add(timer_pop, 0);
        irq_pop();

        schedule();
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