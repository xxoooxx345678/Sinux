#include <usr/shell.h>
#include <drivers/dtb.h>
#include <mm/mm.h>
#include <mm/mmu.h>
#include <kernel/timer.h>
#include <kernel/exception.h>
#include <kernel/sched.h>

extern char *dtb_base_address;

void main()
{
    traverse_device_tree(dtb_callback_initramfs);
    memory_init((void *)PHYS_TO_VIRT(0), (void *)PHYS_TO_VIRT(0x3c000000));
    uart_init();
    timer_init();
    sched_init();
    enable_interrupt();

    thread_create((program_t)shell, "shell");

    sched_enable();

    idle();
}