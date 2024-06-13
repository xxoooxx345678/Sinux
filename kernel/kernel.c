#include <kernel/kernel.h>

extern char *dtb_base_address;

void main()
{
    traverse_device_tree(dtb_callback_initramfs);

    shell();

    while (1);
}