#include <kernel/signal.h>
#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <drivers/uart.h>

#define TBD signal_undefined_default_handler

extern thread_t *cur_thread;

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
    asm volatile("blr x16");
    asm volatile("mov x8, %0\n"::"r"(SYS_SIGRETURN));
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