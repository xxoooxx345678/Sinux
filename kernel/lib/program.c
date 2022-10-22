#include "cpio.h"
#include "program.h"
#include "uart.h"
#include "string.h"

void exec(char *addr)
{
    char *ps = smalloc(0x10000);
//                                           I, bit [7]: IRQ mask
//  asm volatile("mov x1, 0x3c0     \n");    0011 1100 0000
    asm volatile("mov x1, 0x340     \n"); // 0011 0100 0000
    asm volatile("msr spsr_el1, x1     \n");
    asm volatile("msr elr_el1, %0     \n" :: "r" (addr));
    asm volatile("msr sp_el0, %0     \n" :: "r" (ps + 0x10000));
    asm volatile("eret     \n");
}

int loadp(char* thefilepath)
{
    char* filepath;
    char* filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = DTB_START;

    while(header_pointer!=0)
    {
        int error = cpio_newc_parse_header(header_pointer,&filepath,&filesize,&filedata,&header_pointer);
        if(error)
        {
            uart_puts("error");
            break;
        }

        if(strcmp(thefilepath,filepath)==0)
        {
            exec(filedata);
            break;
        }

        if(header_pointer==0)uart_printf("loadp: %s: No such file or directory\r\n",thefilepath);
    }
    return 0;
}
