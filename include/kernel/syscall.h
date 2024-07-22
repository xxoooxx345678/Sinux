#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel/signal.h>
#include <stddef.h>
#include <string.h>

typedef uint64_t (*SYSCALL_FUNC)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

#define MAX_SYSCALL_NUM 64
#define CAST_TO_SYSCALL_FUNC(x) ((SYSCALL_FUNC)(x))

typedef enum {
    SYS_GETPID,
    SYS_UARTREAD,
    SYS_UARTWRITE,
    SYS_EXEC,
    SYS_FORK,
    SYS_EXIT,
    SYS_MBOX_CALL,
    SYS_KILL,
    SYS_SIGNAL,
    SYS_SIGKILL,
    SYS_MMAP,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_WRITE,
    SYS_READ,
    SYS_MKDIR,
    SYS_MOUNT,
    SYS_CHDIR,
    SYS_LSEEK64,
    SYS_IOCTL,
    SYS_SIGRETURN = MAX_SYSCALL_NUM - 1
} SYSCALL_NUM;

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
void *sys_mmap(void* addr, size_t len, int prot, int flags, int fd, int file_offset);
int sys_open(const char *pathname, int flags);
int sys_close(int fd);
long sys_write(int fd, const void *buf, unsigned long count);
long sys_read(int fd, void *buf, unsigned long count);
int sys_mkdir(const char *pathname, unsigned mode);
int sys_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int sys_chdir(const char *path);
long sys_lseek64(int fd, long offset, int whence);
int sys_ioctl(int fd, unsigned long request, void *info);
void sys_sigreturn();

#endif