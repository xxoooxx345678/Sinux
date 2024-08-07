#ifndef TIMER_H
#define TIMER_H

#include <kernel/list.h>
#include <stddef.h>

typedef void (*timer_callback)(char *arg);

typedef struct timer_t {
    list_head_t listhead;
    timer_callback callback;
    char *arg;
    uint64_t timeout;
} timer_t;

void timer_init();
void timer_enable();
void timer_disable();
void timer_interrupt_enable();
void timer_interrupt_disable();
void timer_add(timer_callback callback, char *arg, int bytick, uint64_t timeout);
void timer_pop();

uint64_t get_current_tick();
uint64_t get_clock_freq();

static void set_timer_interrupt_tick(uint64_t tick);
static size_t timer_list_get_size();

#endif