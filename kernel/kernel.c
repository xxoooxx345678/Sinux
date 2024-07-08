#include <kernel/kernel.h>

extern char *dtb_base_address;

struct listitem {
    uint16_t i;
    struct list_head list;
};

void main()
{
    traverse_device_tree(dtb_callback_initramfs);
    memory_init((void *)0, (void *)0x3c000000);
    timer_init();
    uart_init();
    sched_init();
    enable_interrupt();

    thread_create((program_t)shell, "shell");

    sched_enable();

    idle();
}