#include "uart.h"
#include "string.h"
#include "shell.h"
#include "mbox.h"
#include "system.h"
#include "program.h"
#include "cpio.h"
#include "dtb.h"
#include "timer.h"
#include "task.h"
#include "mm.h"

void shell()
{
    char cmd[MAX_BUF_SIZE];
    //print_boot_messages();
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
    unsigned int argc = 0;
    unsigned int cmd_length = strlen(cmd);
    int i, j;

    argv[0] = smalloc(MAX_BUF_SIZE);
    for (i = 0, j = 0; i < cmd_length; ++i)
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

    if (strcmp(cmd, "help") == 0)
    {
        uart_async_printf("help                            : print this help menu\r\n");
        uart_async_printf("hello                           : print Hello World!\r\n");
        uart_async_printf("mailbox                         : show infos of board revision and ARM memory\r\n");
        uart_async_printf("reboot                          : reboot the device\r\n");
        uart_async_printf("ls                              : list directory contents\r\n");
        uart_async_printf("cat [FILE]                      : concatenate files and print on the standard output\r\n");
        uart_async_printf("loadp [FILE]                    : load program and execute a program\r\n");
        uart_async_printf("setTimeout [MESSAGE] [SECONDS]  : print message after [SECONDS] seconds\r\n");
        uart_async_printf("twoSecAlert                     : set an alarm that alert every two seconds\r\n");
        uart_async_printf("testPreemp                      : test preemption\r\n");
        uart_async_printf("malloc [SIZE]                   : memory allocation\r\n");
        uart_async_printf("testPFA                         : test page frame allocator\r\n");
        uart_async_printf("testCSA                         : test chunk slot allocator\r\n");
        uart_async_printf("clear                           : clear page\r\n");
    }
    else if (strcmp(argv[0], "hello") == 0)
        uart_async_printf("Hello World!\r\n");
    else if (strcmp(argv[0], "reboot") == 0)
        reboot();
    else if (strcmp(argv[0], "mailbox") == 0)
        print_system_messages();
    else if (strcmp(argv[0], "ls") == 0)
        ls(".");
    else if (strcmp(argv[0], "cat") == 0)
        cat(argv[1]);
    else if (strcmp(argv[0], "loadp") == 0)
        loadp(argv[1]);
    else if (strncmp(argv[0], "setTimeout", sizeof("setTimeout") - 1) == 0)
        add_timer(uart_puts, atoi(argv[2]), argv[1]);
    else if (strcmp(argv[0], "twoSecAlert") == 0)
        add_timer(two_second_alert, 2, "two_second_alert");
    else if (strcmp(argv[0], "testPreemp") == 0)
        test_preemption();
    else if (strcmp(argv[0], "malloc") == 0)
        malloc(atoi(argv[1]));        
    else if (strcmp(argv[0], "testPFA") == 0)
        page_frame_allocator_test();
    else if (strcmp(argv[0], "testCSA") == 0)
        chunk_slot_allocator_test();
    else if (strcmp(argv[0], "clear") == 0)
        clear();
    else
        uart_async_printf("Unknown command!: %s\r\n", argv[0]);
}

void clear()
{
    uart_async_printf("\x1b[H\x1b[J");
}

void print_boot_messages()
{
    clear();
    uart_async_printf("WELCOME !!!!!\r\n\r\n");
    uart_async_printf("⣿⣿⣿⣿⣿⣿⠿⢋⣥⣴⣶⣶⣶⣬⣙⠻⠟⣋⣭⣭⣭⣭⡙⠻⣿⣿⣿⣿⣿\r\n");
    uart_async_printf("⣿⣿⣿⣿⡿⢋⣴⣿⣿⠿⢟⣛⣛⣛⠿⢷⡹⣿⣿⣿⣿⣿⣿⣆⠹⣿⣿⣿⣿\r\n");
    uart_async_printf("⣿⣿⣿⡿⢁⣾⣿⣿⣴⣿⣿⣿⣿⠿⠿⠷⠥⠱⣶⣶⣶⣶⡶⠮⠤⣌⡙⢿⣿\r\n");
    uart_async_printf("⣿⡿⢛⡁⣾⣿⣿⣿⡿⢟⡫⢕⣪⡭⠥⢭⣭⣉⡂⣉⡒⣤⡭⡉⠩⣥⣰⠂⠹\r\n");
    uart_async_printf("⡟⢠⣿⣱⣿⣿⣿⣏⣛⢲⣾⣿⠃⠄⠐⠈⣿⣿⣿⣿⣿⣿⠄⠁⠃⢸⣿⣿⡧\r\n");
    uart_async_printf("⢠⣿⣿⣿⣿⣿⣿⣿⣿⣇⣊⠙⠳⠤⠤⠾⣟⠛⠍⣹⣛⣛⣢⣀⣠⣛⡯⢉⣰\r\n");
    uart_async_printf("⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡶⠶⢒⣠⣼⣿⣿⣛⠻⠛⢛⣛⠉⣴⣿⣿\r\n");
    uart_async_printf("⣿⣿⣿⣿⣿⣿⣿⡿⢛⡛⢿⣿⣿⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡈⢿⣿\r\n");
    uart_async_printf("⣿⣿⣿⣿⣿⣿⣿⠸⣿⡻⢷⣍⣛⠻⠿⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⢇⡘⣿\r\n");
    uart_async_printf("⣿⣿⣿⣿⣿⣿⣿⣷⣝⠻⠶⣬⣍⣛⣛⠓⠶⠶⠶⠤⠬⠭⠤⠶⠶⠞⠛⣡⣿\r\n");
    uart_async_printf("⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣶⣬⣭⣍⣙⣛⣛⣛⠛⠛⠛⠿⠿⠿⠛⣠⣿⣿\r\n");
    uart_async_printf("⣦⣈⠉⢛⠻⠿⠿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠛⣁⣴⣾⣿⣿⣿⣿\r\n");
    uart_async_printf("⣿⣿⣿⣶⣮⣭⣁⣒⣒⣒⠂⠠⠬⠭⠭⠭⢀⣀⣠⣄⡘⠿⣿⣿⣿⣿⣿⣿⣿\r\n");
    uart_async_printf("⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣦⡈⢿⣿⣿⣿⣿⣿\r\n");
    uart_async_printf("\r\n");
}

void print_system_messages()
{
    unsigned int board_revision;
    get_board_revision(&board_revision);
    uart_async_printf("Board revision is : 0x%x\r\n", board_revision);

    unsigned int arm_mem_base_addr;
    unsigned int arm_mem_size;

    get_arm_memory_info(&arm_mem_base_addr, &arm_mem_size);
    uart_async_printf("ARM memory base address in bytes : 0x%x\r\n", arm_mem_base_addr);
    uart_async_printf("ARM memory size in bytes : 0x%x\r\n", arm_mem_size);
}