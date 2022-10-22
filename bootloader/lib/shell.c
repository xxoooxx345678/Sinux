#include "shell.h"
#include "uart.h"
#include "utils.h"
#include "my_string.h"
#include "fdt.h"

void boot_msg()
{
    uart_printf("\nWELCOME to Bootloader!!!!!\n\n");
}

void cmd_input(char *cmd)
{
    uart_printf("\r# ");
    cmd[0] = 0;
    int idx =  0;
    char c;

    while (1)
    {
        c = uart_getc();
        if ((c >= 32 && c <= 126) || c == '\n' || c == '\r')
        {
            cmd[idx++] = (c == '\n' || c == '\r') ? '\0' : c;
            uart_send(c);
        }
        if (c == '\n' || c == '\r')
        {
            uart_printf("\r");
            if (idx > 1)
                cmd_resolve(cmd);
            uart_printf("# ");
            idx = 0;
        }
    }

}

void cmd_resolve(char *cmd)
{
    if (strcmp(cmd, "help") == 0)
        help();
    else if (strcmp(cmd, "loadimg") == 0)
        loadimg();
    else
        uart_printf("%s: command not found\n", cmd);
}

void loadimg()
{

    char *kernel = (char *)0x80000;
    char buf[8];
    int buf_ptr = 0;
    int size;

again:
    uart_printf("start to load kernel...\n");

    while (1)
    {
        char in_c = uart_getc();

        if (in_c != '\n')
            buf[buf_ptr++] = in_c;
        else
        {
            buf[buf_ptr] = '\0';
            break;
        }
    }

    size = atoi(buf);

    if (size > 1024 * 1024 * 1024)
    {
        // size error
        uart_printf("kernel size error!");
        goto again;
    }

    uart_printf("kernel image size: ");
    uart_printf("%d", size);
    uart_printf("      load addr : 0x80000\n");

    // read the kernel
    int cnt = 0, ttsize = size;
    while (size--)
    {
        kernel[cnt++] = uart_getcR();
        if (cnt % 1000 == 0)
            uart_printf("Received %d bytes,\t%f%% has completed\n", cnt, ((float)cnt) / ttsize * 100);
    }
    uart_printf("Received %d bytes,\t%f%% has completed\n", cnt, ((float)cnt) / ttsize * 100);
    void (*boot)(unsigned long) = (void *)kernel;
    boot(DTB_BASE);
}

void help()
{
    uart_printf("help\t: print this help menu\n");
    uart_printf("loadimg\t: load kernel image through UART\n");

    uart_printf("\n");
    uart_printf("⠄⠰⠛⠋⢉⣡⣤⣄⡉⠓⢦⣀⠙⠉⠡⠔⠒⠛⠛⠛⠶⢶⣄⠘⢿⣷⣤⡈⠻⣧\n");
    uart_printf("⢀⡔⠄⠄⠄⠙⣿⣿⣿⣷⣤⠉⠁⡀⠐⠒⢿⣿⣿⣿⣶⣄⡈⠳⢄⣹⣿⣿⣾⣿\n");
    uart_printf("⣼⠁⢠⡄⠄⠄⣿⣿⣿⣿⡟⠄⡐⠁⡀⠄⠈⣿⣿⣿⣿⣿⣷⣤⡈⠻⣿⣿⣿⣿\n");
    uart_printf("⢻⡀⠈⠄⠄⣀⣿⣿⣿⡿⠃⠄⡇⠈⠛⠄⠄⣿⣿⣿⣿⣿⣿⠟⠋⣠⣶⣿⣿⣿\n");
    uart_printf("⠄⢉⡓⠚⠛⠛⠋⣉⣩⣤⣤⣀⠑⠤⣤⣤⣾⣿⣿⣿⡿⠛⢁⣤⣾⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⠈⠙⠛⠋⣭⣭⣶⣾⣿⣿⣿⣷⣦⢠⡍⠉⠉⢠⣤⣴⠚⢩⣴⣿⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⢴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣦⣭⣭⣭⣥⣴⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⣴⣶⡶⠶⠶⠶⠶⠶⠶⠶⠶⣮⣭⣝⣛⠿⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⠙⣿⡄⠄⠄⢀⡤⠬⢭⣝⣒⢂⠭⣉⠻⠿⣷⣶⣦⣭⡛⣿⣿⣿⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⠄⠸⣿⡇⠄⠸⣎⣁⣾⠿⠉⢀⠇⣸⣿⣿⢆⡉⠹⣿⣿⢸⣿⣿⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⠄⠄⣿⡇⠄⢀⡶⠶⠶⠾⠿⠮⠭⠭⢭⣥⣿⣿⣷⢸⣿⢸⣿⣿⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⠄⠄⣿⡇⠄⠈⣷⠄⠄⠄⣭⣙⣹⢙⣰⡎⣿⢏⣡⣾⢏⣾⣿⣿⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⠄⢰⣿⡇⠄⠄⢿⠄⠄⠈⣿⠉⠉⣻⣿⡷⣰⣿⡿⣡⣿⣿⣿⣿⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⠄⢸⣿⡇⠄⠄⠘⠿⠤⠤⠿⠿⠿⢤⣤⣤⡿⣃⣸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n");
    uart_printf("⠄⠄⠘⢿⣷⣤⣄⣀⣀⣀⣀⣀⣠⣴⣾⡿⢋⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋\n");
}
