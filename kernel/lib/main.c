#include "uart.h"
#include "mbox.h"
#include "shell.h"
#include "string.h"
#include "malloc.h"
#include "dtb.h"
#include "cpio.h"
#include "exception.h"
#include "task.h"
#include "mm.h"

extern char* dtb_place;

void main(char* dtb)
{
    dtb_place = dtb;

    task_list_init();
    timer_list_init();
    core_timer_enable();
    enable_mini_uart_interrupt();
    enable_interrupt();

    traverse_device_tree(dtb_place, dtb_callback_initramfs);

    memory_init();
    shell();
}