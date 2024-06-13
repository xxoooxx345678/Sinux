#include <usr/shell.h>

void shell()
{
    char cmd[MAX_BUF_SIZE];
    print_boot_msg();
    while (1)
    {
        uart_printf("# ");
        uart_gets(cmd);
        cmd_resolve(cmd);
    }
}

void cmd_resolve(char *cmd)
{
    char argv[100][MAX_BUF_SIZE];
    size_t argc = 0;
    size_t cmd_len = strlen(cmd);

    int i, j;
    for (i = 0, j = 0; i < cmd_len; ++i)
    {
        if (cmd[i] == ' ')
        {
            argv[argc][j] = '\0';
            j = 0;
            continue;
        }
        argv[argc][j++] = cmd[i];
    }
    argv[argc++][j] = '\0';

    if (!strcmp(cmd, "help"))
    {
        uart_printf("help                            : print this help menu\n");
        uart_printf("load                            : load the kernel image\n");
    }
    else if (!strcmp(cmd, "load"))
        load();
    else
        uart_printf("Unknown command!: %s\n", argv[0]);
}

void clear()
{
    uart_printf("\x1b[H\x1b[J");
}

void print_boot_msg()
{
    // clear();
    uart_printf("WELCOME to bootloader !!!!!\n");
    uart_printf("\n");
}