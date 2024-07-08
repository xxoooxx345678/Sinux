#ifndef SIGNAL_H
#define SIGNAL_H

#include <kernel/sched.h>
#include <drivers/uart.h>

void registered_signal_handler_wrapper();
void signal_kill_default_handler(int signal);
void signal_undefined_default_handler(int signal);

#endif