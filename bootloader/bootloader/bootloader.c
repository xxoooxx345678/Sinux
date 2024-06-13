#include <usr/shell.h>
#include <load.h>

int relocated = 0;

void main()
{
    if (relocated == 0)
    {
        relocated = 1;
        relocate();
    }

    shell();
}