#ifndef TIMER_H
#define TIMER_H

#include "list.h"
#define CORE0_TIMER_IRQ_CTRL 0x40000040

#define STR(x) #x
#define XSTR(s) STR(s)

typedef struct timer_event
{
    struct list_head listhead;
    unsigned long long expire_time; 
    void *callback; 
    char *args; 
} timer_event_t;

void timer_list_init();

void core_timer_enable();
void core_timer_disable();
void core_timer_handler();

//now the callback only support "funcion(char *)", char* in args
void add_timer(void *callback, unsigned long long timeout, char *args);
void set_core_timer_interrupt(unsigned long long expired_time);
void disable_core_timer_interrupt();
unsigned long long get_tick_plus_s(unsigned long long second);
void set_core_timer_interrupt_by_tick(unsigned long long tick);
void two_second_alert(char *str);

#endif