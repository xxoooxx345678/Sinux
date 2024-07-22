#include <usr/shell.h>
#include <drivers/dtb.h>
#include <drivers/uart.h>
#include <mm/mm.h>
#include <mm/mmu.h>
#include <kernel/timer.h>
#include <kernel/exception.h>
#include <kernel/sched.h>
#include <fs/vfs.h>

void run_TA_img()
{
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));

    uart_disable_echo();
    thread_exec("./initramfs/vfs1.img", NULL);
}

void main()
{
    traverse_device_tree(dtb_callback_initramfs);
    memory_init((void *)PHYS_TO_VIRT(0), (void *)PHYS_TO_VIRT(0x3c000000));
    fs_init();
    uart_init();
    timer_init();
    sched_init();
    enable_interrupt();

    // thread_create((program_t)shell, "shell");
    thread_create((program_t)run_TA_img, "TA_IMG");

    sched_enable();

    // make current thread an idle thread
    idle();
}