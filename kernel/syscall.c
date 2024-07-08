#include <kernel/syscall.h>

extern thread_t *cur_thread;
extern thread_t threads[MAX_THREAD_COUNT];

int sys_getpid()
{
    return cur_thread->pid; 
}

size_t sys_uartread(char buf[], size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i)
        buf[i] = uart_async_getc();
    
    return i;
}

size_t sys_uartwrite(const char buf[], size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i)
        uart_async_putc(buf[i]);

    return i;
}

int sys_exec(const char *name, char *const argv[])
{
    return thread_exec(name, argv);
}

int sys_fork()
{
    CRITICAL_SECTION_START;

    thread_t *child_thread = thread_create(cur_thread->entry_point, cur_thread->name);
    
    int parent_pid = cur_thread->pid;
    thread_t *parent_thread = cur_thread;

    memcpy(child_thread->user_sp, cur_thread->user_sp, USTACK_SIZE);
    memcpy(child_thread->kernel_sp, cur_thread->kernel_sp, KSTACK_SIZE);

    for (int i = 0; i < MAX_SIGNAL_HANDLER_CNT; ++i)
    {
        child_thread->registered_signal_handler[i] = cur_thread->registered_signal_handler[i];
        child_thread->signal_pending_count[i] = cur_thread->signal_pending_count[i];
    }

    store_context(&cur_thread->context);

    if (parent_pid != cur_thread->pid)
        goto child;

    child_thread->context = cur_thread->context;
    child_thread->context.fp = child_thread->kernel_sp + (cur_thread->context.fp - (uint64_t)cur_thread->kernel_sp);
    child_thread->context.sp = child_thread->kernel_sp + (cur_thread->context.sp - (uint64_t)cur_thread->kernel_sp);

    CRITICAL_SECTION_END;
    return child_thread->pid;

child:
    cur_thread->trapframe = (trapframe_t *)((uint64_t)cur_thread->kernel_sp + (uint64_t)parent_thread->trapframe - (uint64_t)parent_thread->kernel_sp);
    cur_thread->trapframe->sp_el0 = cur_thread->user_sp + cur_thread->trapframe->sp_el0 - parent_thread->user_sp;
    return 0;
}

void sys_exit()
{
    thread_exit();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox)
{
    return mbox_call(ch, mbox);
}

void sys_kill(int pid)
{
    CRITICAL_SECTION_START;

    if (pid >= MAX_THREAD_COUNT || pid < 0 || threads[pid].status == FREE)
    {
        CRITICAL_SECTION_END;
        return;
    }

    threads[pid].status = DEAD;

    CRITICAL_SECTION_END;
}

void sys_signal(int signal, void (*signal_handler)(int))
{
    CRITICAL_SECTION_START;
    cur_thread->registered_signal_handler[signal] = signal_handler;
    CRITICAL_SECTION_END;
}

void sys_sigkill(int pid, int signal)
{
    CRITICAL_SECTION_START;
    threads[pid].signal_pending_count[signal]++;
    CRITICAL_SECTION_END;
}

void sys_sigreturn()
{
    free(cur_thread->signal_handler_stack);
    load_context(&cur_thread->signal_context);
}