#ifndef TASK_H
#define TASK_H

#include "list.h"

#define UART_IRQ_PRIORITY 1
#define TIMER_IRQ_PRIORITY -1

typedef struct task
{
    struct list_head listhead;
    int nice; 
    void *task_function;
} task_t;

void task_list_init();
void add_task(void *task_function, int nice);
void pop_task();
void run_task(task_t *the_task);
void test_preemption();

#endif