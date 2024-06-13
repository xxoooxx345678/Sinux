#include <load.h>

#define NEW_BL_START 0x60000

extern char __start[];
extern char __end[];

extern char *dtb_base_address;

void relocate()
{
    char *cur_bl_start = (char *)__start;
    char *cur_bl_end   = (char *)__end;
    int bl_size = cur_bl_end - cur_bl_start;
    char *new_bl_start = (char *)NEW_BL_START;

    for (int i = 0; i < bl_size; ++i)
        new_bl_start[i] = cur_bl_start[i];
    
    void (*new_bl)() = (void (*)())new_bl_start;
    new_bl();
}

void load()
{
    char *backup_dtb_base_address = dtb_base_address;

    char buf[MAX_BUF_SIZE];
    int img_size;

    uart_printf("Enter kernel image size: ");
    uart_gets(buf);
    img_size = atoi(buf);

    char *kernel_start = (char *)0x80000;

    uart_printf("\nStart transfering ... ");
    for (int i = 0; i < img_size; ++i)
        kernel_start[i] = uart_raw_getc();

    void (*kernel)(char *) = (void(*)(char *))kernel_start;
    kernel(backup_dtb_base_address);
}