#include <kernel/exception.h>
#include <drivers/uart.h>
#include <drivers/mmio.h>
#include <kernel/timer.h>
#include <kernel/syscall.h>
#include <kernel/trapframe.h>
#include <kernel/sched.h>
#include <mm/mm.h>
#include <mm/vm.h>
#include <stddef.h>

extern const SYSCALL_FUNC sys_table[];

extern thread_t *cur_thread;

static int lock = 0;

LIST_HEAD(irq_list);

int curr_irq_prio = 32768;

exception_t get_current_exception()
{
    exception_t ex;

    asm volatile("mrs %0, esr_el1":"=r"(ex));

    return ex;
}

void irq_add(irq_callback callback, int prio)
{
    irq_t *irq = malloc(sizeof(irq_t));

    // init listhead
    INIT_LIST_HEAD(&irq->listhead);    

    // set callback & prio
    irq->callback = callback;
    irq->prio = prio;

    list_head_t *it = NULL;
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

static void syscall_handler(trapframe_t *tf)
{
    enable_interrupt();
    cur_thread->trapframe = tf;

    uint64_t syscall_no = cur_thread->trapframe->x8;

    cur_thread->trapframe->x0 = sys_table[syscall_no](cur_thread->trapframe->x0, cur_thread->trapframe->x1, cur_thread->trapframe->x2, cur_thread->trapframe->x3, cur_thread->trapframe->x4);
}

void sync_el0_64_handler(trapframe_t *tf)
{
    exception_t ex = get_current_exception();

    switch (ex.ec)
    {
    case SVC_INS_EXEC:
        syscall_handler(tf);
        break;
    case DATA_ABORT_LOWER:
    case INS_ABORT_LOWER:
        page_fault_handler(&ex);
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
    exception_t ex = get_current_exception();
    int level;
    uart_printf("From: 0x%x\n", x0);
    uart_printf("Exception level: %d\n", current_exception_level());
    uart_printf("ISS: 0x%x\n", ex.iss);

    switch (ex.ec)
    {
    case 0b011000:
        uart_printf("Trapped MSR, MRS, or System instruction execution\n");
        break;
    case 0b011001:
        uart_printf("Trapped access to SVE functionality\n");
        break;
    case 0b100000:
        uart_printf("Instruction Abort from a lower Exception Level\n");
        break;
    case 0b100001:
        uart_printf("Instruction Abort taken without a change in Exception level\n");
        break;
    case 0b100101:
        uart_printf("Data Abort taken without a change in Exception level (invalid data access)\n");
        break;
    default:
        uart_printf("EC: 0x%x\n", ex.ec);
        break;
    }
}

void critical_section_end()
{
    --lock;
    if (lock == 0)
        enable_interrupt();
}

void critical_section_start()
{
    ++lock;
    disable_interrupt();
}