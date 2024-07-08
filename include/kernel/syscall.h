#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel/sched.h>
#include <mm/mm.h>
#include <drivers/uart.h>
#include <drivers/mailbox.h>
#include <fs/cpio.h>
#include <string.h>

int sys_getpid();
size_t sys_uartread(char buf[], size_t size);
size_t sys_uartwrite(const char buf[], size_t size);
int sys_exec(const char *name, char *const argv[]);
int sys_fork();
void sys_exit();
int sys_mbox_call(unsigned char ch, unsigned int *mbox);
void sys_kill(int pid);
void sys_signal(int signal, void (*signal_handler)(int));
void sys_sigkill(int pid, int signal);
void sys_sigreturn();

#endif