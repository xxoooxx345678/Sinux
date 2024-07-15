#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/exception.h>
#include <mm/mm.h>
#include <mm/mmu.h>
#include <mm/vm.h>
#include <drivers/uart.h>
#include <drivers/mailbox.h>
#include <fs/cpio.h>

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
    int ret = 0;
    CRITICAL_SECTION_START;

    thread_t *child_thread = thread_create(cur_thread->entry_point, cur_thread->name);

    memcpy(child_thread->kernel_sp, cur_thread->kernel_sp, KSTACK_SIZE);

    for (int i = 0; i < MAX_SIGNAL_HANDLER_CNT; ++i)
    {
        child_thread->registered_signal_handler[i] = cur_thread->registered_signal_handler[i];
        child_thread->signal_pending_count[i] = cur_thread->signal_pending_count[i];
    }

    copy_mapping(child_thread, cur_thread);

    store_context(&cur_thread->context);

    void *ttbr0_el1 = child_thread->context.ttbr0_el1;
    child_thread->context = cur_thread->context;
    child_thread->context.lr = &&out;
    child_thread->context.fp = child_thread->kernel_sp + (cur_thread->context.fp - (uint64_t)cur_thread->kernel_sp);
    child_thread->context.sp = child_thread->kernel_sp + (cur_thread->context.sp - (uint64_t)cur_thread->kernel_sp);
    child_thread->trapframe = (trapframe_t *)((uint64_t)child_thread->kernel_sp + (uint64_t)cur_thread->trapframe - (uint64_t)cur_thread->kernel_sp);
    child_thread->context.ttbr0_el1 = ttbr0_el1;

    ret = child_thread->pid;

    CRITICAL_SECTION_END;

out:
    return ret;
}

void sys_exit()
{
    thread_exit();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox)
{
    unsigned int __attribute__((aligned(16))) _mbox[36];

    unsigned int mbox_size = mbox[0];
    memcpy(_mbox, mbox, mbox_size);
    int ret = mbox_call(ch, _mbox);
    memcpy(mbox, _mbox, mbox_size);

    return ret;
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

void sys_signal(int signal, signal_handler_t signal_handler)
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
    uint64_t *pte = (uint64_t *)walk(PHYS_TO_VIRT(cur_thread->context.ttbr0_el1), USER_SIG_STACK_VIRT_ADDR_ALIGNED, 0);
    dec_ref_count(PHYS_TO_VIRT((*pte) & PD_ADDR_MASK));
    free(PHYS_TO_VIRT((*pte) & PD_ADDR_MASK));
    *pte = 0;

    load_context(&cur_thread->signal_context);
}

void *sys_mmap(void* addr, size_t len, int prot, int flags, int fd, int file_offset)
{
    return mappages(cur_thread, (uint64_t)addr, len, 0, prot);
}
