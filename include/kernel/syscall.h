#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel/signal.h>
#include <stddef.h>
#include <string.h>

int sys_getpid();
size_t sys_uartread(char buf[], size_t size);
size_t sys_uartwrite(const char buf[], size_t size);
int sys_exec(const char *name, char *const argv[]);
int sys_fork();
void sys_exit();
int sys_mbox_call(unsigned char ch, unsigned int *mbox);
void sys_kill(int pid);
void sys_signal(int signal, signal_handler_t signal_handler);
void sys_sigkill(int pid, int signal);
void sys_sigreturn();
void *sys_mmap(void* addr, size_t len, int prot, int flags, int fd, int file_offset);

#endif