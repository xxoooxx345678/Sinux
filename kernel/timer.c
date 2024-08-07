#include <kernel/timer.h>
#include <kernel/exception.h>
#include <mm/mmu.h>
#include <mm/mm.h>
#include <string.h>

#define CORE0_TIMER_IRQ_CTRL PHYS_TO_VIRT(0x40000040)

LIST_HEAD(timer_list);

void timer_init()
{
    timer_enable();
    timer_interrupt_enable();
}

void timer_enable()
{
    asm volatile("msr   cntp_ctl_el0, %0"::"r"(1));
}

void timer_disable()
{
    asm volatile("msr   cntp_ctl_el0, %0"::"r"(0));
}

void timer_interrupt_enable()
{
    asm volatile("str   %0, [%1]"::"r"(0b10), "r"(CORE0_TIMER_IRQ_CTRL));
}

void timer_interrupt_disable()
{
    asm volatile("str   %0, [%1]"::"r"(0b00), "r"(CORE0_TIMER_IRQ_CTRL));
}

void timer_add(timer_callback callback, char *arg, int bytick, uint64_t timeout)
{
    timer_t *timer = malloc(sizeof(timer_t));

    // init listhead
    INIT_LIST_HEAD(&timer->listhead);

    // copy callback & arguments
    timer->callback = callback;
    timer->arg = malloc(strlen(arg) + 1);
    strcpy(timer->arg, arg);

    // set timeout
    timer->timeout = get_current_tick() + (bytick ? timeout : timeout * get_clock_freq());
    list_head_t *it = NULL;

    CRITICAL_SECTION_START; 
    list_for_each(it, &timer_list)
    {
        if (((timer_t *)it)->timeout > timer->timeout)
        {
            list_add(&timer->listhead, it->prev);
            break;
        }
    }

    // Furthest timeout
    if (list_is_head(it, &timer_list))
        list_add_tail(&timer->listhead, &timer_list);

    // Update timer interrupt tick
    set_timer_interrupt_tick(((timer_t *)timer_list.next)->timeout);
    CRITICAL_SECTION_END; 

    timer_interrupt_enable();
}

void timer_pop()
{
    if (list_empty(&timer_list))
        return;

    timer_t *head = (timer_t *)timer_list.next;
    
    CRITICAL_SECTION_START; 
    list_del_entry(timer_list.next);
    CRITICAL_SECTION_END; 

    head->callback(head->arg);

    CRITICAL_SECTION_START; 
    free(head->arg);
    free(head);
    CRITICAL_SECTION_END; 

    // unmasks the interrupt line to get the next interrupt at the end of the task.
    timer_interrupt_enable();
}

uint64_t get_current_tick()
{
    uint64_t ret;

    asm volatile("mrs   %0, cntpct_el0": "=r"(ret));
    return ret;
}

uint64_t get_clock_freq()
{
    uint64_t ret;

    asm volatile("mrs   %0, cntfrq_el0": "=r"(ret));
    return ret;
}

static void set_timer_interrupt_tick(uint64_t tick)
{
    asm volatile("msr   cntp_cval_el0, %0"::"r"(tick));
}

static size_t timer_list_get_size()
{
    int r = 0;
    list_head_t *it = NULL;
    list_for_each(it, &timer_list)
    {
        r++;
    }
    return r;
}