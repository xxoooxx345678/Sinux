#include "timer.h"
#include "uart.h"
#include "malloc.h"
#include "string.h"

struct list_head *timer_event_list;

void timer_list_init()
{
    INIT_LIST_HEAD(timer_event_list);
}

void core_timer_enable()
{
    asm volatile(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t"
        "mov x2, 2\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t"
    );
}

void core_timer_disable()
{
    asm volatile(
        "mov x2, 0\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t" 
    );
}

void timer_event_callback(timer_event_t *timer_event)
{

    list_del_entry((struct list_head *)timer_event); 
    ((void (*)(char *))timer_event->callback)(timer_event->args); 

    if (list_empty(timer_event_list))
        disable_core_timer_interrupt();
    else
        set_core_timer_interrupt_by_tick(((timer_event_t *)timer_event_list->next)->expire_time);
}

void two_second_alert(char *str)
{
    unsigned long long cntpct_el0;
    asm volatile("mrs %0, cntpct_el0\n\t"
                         : "=r"(cntpct_el0));

    unsigned long long cntfrq_el0;
    asm volatile("mrs %0, cntfrq_el0\n\t"
                         : "=r"(cntfrq_el0));
    uart_printf("'%s': seconds after booting : %d\r\n", str, cntpct_el0 / cntfrq_el0);

    add_timer(two_second_alert, 2, "two_second_alert");
}

void core_timer_handler()
{
    if (list_empty(timer_event_list))
    {
        disable_core_timer_interrupt();
        return;
    }

    timer_event_callback((timer_event_t *)timer_event_list->next);
}

void add_timer(void *callback, unsigned long long timeout, char *args)
{

    timer_event_t *the_timer_event = smalloc(sizeof(timer_event_t));

    the_timer_event->args = smalloc(strlen(args) + 1);
    strcpy(the_timer_event->args, args);

    the_timer_event->expire_time = get_tick_plus_s(timeout);
    the_timer_event->callback = callback;
    INIT_LIST_HEAD(&the_timer_event->listhead);

    struct list_head *curr;

    list_for_each(curr, timer_event_list)
    {
        if (((timer_event_t *)curr)->expire_time > the_timer_event->expire_time)
        {
            list_add(&the_timer_event->listhead, curr->prev);
            break;
        }
    }

    if (list_is_head(curr, timer_event_list))
        list_add_tail(&the_timer_event->listhead, timer_event_list);

    set_core_timer_interrupt_by_tick(((timer_event_t *)timer_event_list->next)->expire_time);
}

void set_core_timer_interrupt(unsigned long long expired_time)
{
    asm volatile(
        "mrs x1, cntfrq_el0\n\t"
        "mul x1, x1, %0\n\t"
        "msr cntp_tval_el0, x1\n\t"
        : "=r"(expired_time));
}

void disable_core_timer_interrupt()
{
    set_core_timer_interrupt(10000000);
}

unsigned long long get_tick_plus_s(unsigned long long second)
{

    unsigned long long cntpct_el0 = 0;
    asm volatile("mrs %0, cntpct_el0\n\t"
                         : "=r"(cntpct_el0)); 

    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0\n\t"
                         : "=r"(cntfrq_el0)); 

    return (cntpct_el0 + cntfrq_el0 * second);
}

void set_core_timer_interrupt_by_tick(unsigned long long tick)
{
    asm volatile(
        "msr cntp_cval_el0, %0\n\t" 
        : "=r"(tick));
}