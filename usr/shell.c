#include <usr/shell.h>

static void timer_test()
{
    uart_async_printf("%d seconds after booting\n", get_current_tick() / get_clock_freq());
    timer_add(timer_test, NULL, 2);
}

void shell()
{
    char cmd[MAX_BUF_SIZE];
    print_boot_msg();
    while (1)
    {
        uart_async_printf("# ");
        uart_async_gets(cmd);
        cmd_resolve(cmd);
    }
}

void cmd_resolve(char *cmd)
{
    char *argv[100];
    size_t argc = 0;
    size_t cmd_len = strlen(cmd);

    argv[0] = smalloc(MAX_BUF_SIZE);

    int i, j;
    for (i = 0, j = 0; i < cmd_len; ++i)
    {
        if (cmd[i] == ' ')
        {
            argv[argc][j] = '\0';
            argv[++argc] = smalloc(MAX_BUF_SIZE);
            j = 0;
            continue;
        }
        argv[argc][j++] = cmd[i];
    }
    argv[argc++][j] = '\0';

    if (!strcmp(argv[0], "help"))
    {
        uart_async_printf("help                            : print this help menu\n");
        uart_async_printf("hello                           : print Hello World!\n");
        uart_async_printf("info                            : show infos of board revision and ARM memory\n");
        uart_async_printf("ls                              : list directory contents\n");
        uart_async_printf("cat [FILE]                      : concatenate files and print on the standard output\n");
        uart_async_printf("exec [FILE]                     : load program and execute a program\n");
        uart_async_printf("setTimeout [MESSAGE] [SECONDS]  : print [MESSAGE] after [SECONDS] seconds\n");
        uart_async_printf("timer_test                      : set an alarm that alert every two seconds\n");
    }
    else if (!strcmp(argv[0], "hello"))
        uart_async_printf("Hello World!\n");
    else if (!strcmp(argv[0], "clear"))
        clear();
    else if (!strcmp(argv[0], "info"))
    {
        unsigned int board_revision;
        get_board_revision(&board_revision);
        uart_async_printf("Board revision is : 0x%x\n", board_revision);

        unsigned int arm_mem_base_addr;
        unsigned int arm_mem_size;

        get_arm_memory_info(&arm_mem_base_addr, &arm_mem_size);
        uart_async_printf("ARM memory base address in bytes : 0x%x\n", arm_mem_base_addr);
        uart_async_printf("ARM memory size in bytes : 0x%x\n", arm_mem_size);
    }
    else if (!strcmp(argv[0], "ls"))
        ls(".");
    else if (!strcmp(argv[0], "cat"))
        cat(argv[1]);
    else if (!strcmp(argv[0], "exec"))
    {
        char *program_start = get_file_start(argv[1]);
        char *user_stack = smalloc(0x100) + 0x100;
        run_program(program_start, user_stack);
    }
    else if (!strcmp(argv[0], "setTimeout"))
        timer_add(uart_async_puts, argv[1], atoi(argv[2]));
    else if (!strcmp(argv[0], "timer_test"))
        timer_add(timer_test, NULL, 2);
    else
        uart_async_printf("Unknown command!: %s\n", argv[0]);
}

void clear()
{
    uart_async_printf("\x1b[H\x1b[J");
}

void print_boot_msg()
{
    clear();
    uart_async_printf("WELCOME !!!!!\n");
    uart_async_printf("\n");
}