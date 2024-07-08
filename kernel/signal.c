#include <kernel/signal.h>

#define TBD signal_undefined_default_handler

signal_handler_t default_signal_handler_table[MAX_SIGNAL_HANDLER_CNT] = {
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    signal_kill_default_handler,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD,
    TBD
};

void registered_signal_handler_wrapper()
{
    get_current_thread()->handling_signal(0);

    asm volatile("mov x8,30\n");
    asm volatile("svc 0\n");
}

void signal_kill_default_handler(int signal)
{
    thread_exit();
}

void signal_undefined_default_handler(int signal)
{
    uart_printf("undefined signal handler...\n");
}