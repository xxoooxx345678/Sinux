#ifndef SIGNAL_H
#define SIGNAL_H

#define MAX_SIGNAL_HANDLER_CNT  32

typedef void (*signal_handler_t)(int);

void registered_signal_handler_wrapper();
void signal_kill_default_handler(int signal);
void signal_undefined_default_handler(int signal);

#endif