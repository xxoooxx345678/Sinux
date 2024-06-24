#include <kernel/kernel.h>

extern char *dtb_base_address;

void main()
{
    traverse_device_tree(dtb_callback_initramfs);
    memory_init((void *)0, (void *)0x3c000000);
    timer_init();
    uart_init();
    enable_interrupt();

    shell();

    while (1);
}