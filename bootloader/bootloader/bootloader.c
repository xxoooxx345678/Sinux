#include <usr/shell.h>
#include <load.h>

int relocated = 0;
char *dtb_base_address;

void main(char *addr)
{
    if (relocated == 0)
    {
        dtb_base_address = addr;
        relocated = 1;
        relocate();
    }


    shell();
}