#include "task.h"
#include "exception.h"
#include "string.h"
#include "malloc.h"
#include "uart.h"

int curr_task_nice = 123456; 

struct list_head *task_list;

void task_list_init()
{
    INIT_LIST_HEAD(task_list);
}

void add_task(void *task_function, int nice){
    task_t *the_task = smalloc(sizeof(task_t));

    the_task->nice = nice;
    the_task->task_function = task_function;
    INIT_LIST_HEAD(&the_task->listhead);

    struct list_head *curr;

    disable_interrupt(); 
    list_for_each(curr, task_list)
    {
        if (((task_t *)curr)->nice > the_task->nice)
        {
            list_add(&the_task->listhead, curr->prev);
            break;
        }
    }

    if (list_is_head(curr, task_list))
        list_add_tail(&the_task->listhead, task_list); 
    
    enable_interrupt();
}

void pop_task(){
    enable_interrupt(); 
    while (!list_empty(task_list))
    {
        disable_interrupt();
        task_t *the_task = (task_t *)task_list->next;
        if (curr_task_nice <= the_task->nice) // curr task has higher priority
        {
            enable_interrupt();
            break;
        }
        list_del_entry((struct list_head *)the_task);
        int prev_task_nice = curr_task_nice;
        curr_task_nice = the_task->nice;
        enable_interrupt();

        run_task(the_task);

        disable_interrupt();
        curr_task_nice = prev_task_nice;
        enable_interrupt();
    }
}

void run_task(task_t* the_task)
{
    ((void (*)())the_task->task_function)();
}

void highp()
{
    uart_async_printf("high prior start\r\n");
    uart_async_printf(" high prior end\r\n");
}

void lowp()
{
    uart_async_printf("low prior start\r\n");
    add_task(highp, 0);
    for (unsigned long long i = 0; i < 10000000; ++i);
    uart_async_printf("  low prior end\r\n");
}

void test_preemption()
{
    add_task(lowp, 9);
}