#include <string.h>
#include <drivers/uart.h>

extern char __bss_start[], __bss_end[];

void bssinit()
{
    memset(__bss_start, 0, __bss_end - __bss_start);
}
