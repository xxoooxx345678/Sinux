#include <drivers/uart.h>

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int *)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO       ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER      ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int *)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int *)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int *)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int *)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int *)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int *)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int *)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int *)(MMIO_BASE + 0x00215068))

char uart_tx_buffer[MAX_BUF_SIZE] = {0};
uint16_t uart_tx_buffer_w_idx = 0;
uint16_t uart_tx_buffer_r_idx = 0; 

char uart_rx_buffer[MAX_BUF_SIZE] = {0};
uint16_t uart_rx_buffer_w_idx = 0;
uint16_t uart_rx_buffer_r_idx = 0;

void uart_init()
{
    register unsigned int r;

    /* map UART1 to GPIO pins */
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15)); // clean gpio14, gpio15
    r |= (2 << 12) | (2 << 15);    // set alt5 for gpio14, gpio15
    *GPFSEL1 = r;
    *GPPUD = 0; // disable pull up/down for gpio14, gpio15
    r = 150;
    while (r--) // wait 150 cycles
        asm volatile("nop");
    *GPPUDCLK0 = (1 << 14) | (1 << 15); // Set Clock on line 14, 15
    r = 150;
    while (r--) // wait 150 cycles
        asm volatile("nop");
    *GPPUDCLK0 = 0;   // flush GPIO setup

    /* initialize UART */
    *AUX_ENABLE |= 1;   // 1.Set AUXENB register to enable mini UART. Then mini UART register can be accessed.
    *AUX_MU_CNTL = 0;   // 2.Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
    *AUX_MU_IER = 0;    // 3.Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
    *AUX_MU_LCR = 3;    // 4.Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
    *AUX_MU_MCR = 0;    // 5.Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
    *AUX_MU_BAUD = 270; // 6.Set AUX_MU_BAUD to 270. Set baud rate to 115200
    *AUX_MU_IIR = 0x6;  // 7.Set AUX_MU_IIR_REG to 6. No FIFO. (not clear FIFO ???)
    *AUX_MU_CNTL = 3;   // 8.Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.

    /* Enable UART interrupt */
    uart_enable_tx_interrupt();
    uart_enable_rx_interrupt();
    *ENABLE_IRQS_1 |= (1 << 29);
}

void uart_enable_tx_interrupt()
{
    *AUX_MU_IER |= 0b10;
}

void uart_disable_tx_interrupt()
{
    *AUX_MU_IER &= ~0b10;
}

void uart_enable_rx_interrupt()
{
    *AUX_MU_IER |= 0b01;
}

void uart_disable_rx_interrupt()
{
    *AUX_MU_IER &= ~0b01;
}

void uart_putc(char c)
{
    /* wait until we can send */
    do
    {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = (unsigned int)c;
}

void uart_puts(char *s)
{
    int i = 0;

    while (*s)
    {
        uart_putc(*s++);
        i++;
    }
    uart_putc('\r');
    uart_putc('\n');
}

char uart_getc()
{
    char r;
    /* wait until something is in the buffer */
    do
    {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    r = (char)(*AUX_MU_IO);

    /* echo back */
    if (r == '\r')
    {
        uart_printf("\r\r\n");
        do
        {
            asm volatile("nop");
        } while (!(*AUX_MU_LSR & 0x40)); // wait for output success Transmitter idle
    }
    else if (r == '\x7f') // backspace
        uart_printf("\b \b");
    else
        uart_putc(r);
    /* convert carriage return to newline */
    return r == '\r' ? '\n' : r;
}

char *uart_gets(char *buf)
{
    int count;
    char c;
    char *s;
    for (s = buf, count = 0; (c = uart_getc()) != '\n' && count != MAX_BUF_SIZE - 1; count++)
    {
        *s = c;
        if (*s == '\x7f')
        {
            count--;
            if (count == -1)
            {
                uart_putc(' '); // prevent back over command line #
                continue;
            }
            s--;
            count--;
            continue;
        }
        s++;
    }
    *s = '\0';
    return buf;
}

int uart_printf(char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[MAX_BUF_SIZE];
    char *s = (char *)buf;
    int count = vsprintf(s, fmt, args);
    while (*s)
    {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
    __builtin_va_end(args);
    return count;
}

void uart_interrupt_handler()
{
    if (*AUX_MU_IIR & (0b01 << 1)) // tx interrupt
    {
        uart_disable_tx_interrupt();
        uart_tx_interrupt_handler();
    }

    if (*AUX_MU_IIR & (0b10 << 1)) // rx interrupt
    {
        uart_disable_rx_interrupt();
        uart_rx_interrupt_handler();
    }
}

static void uart_tx_interrupt_handler()
{
    if (uart_tx_buffer_r_idx == uart_tx_buffer_w_idx)
    {
        uart_disable_tx_interrupt();
        return;
    }

    uart_putc(uart_tx_buffer[uart_tx_buffer_r_idx++]);
    uart_tx_buffer_r_idx %= MAX_BUF_SIZE;

    uart_enable_tx_interrupt(); // unmasks the interrupt line to get the next interrupt at the end of the task.
}

static void uart_rx_interrupt_handler()
{
    uart_rx_buffer[uart_rx_buffer_w_idx++] = uart_getc();
    uart_rx_buffer_w_idx %= MAX_BUF_SIZE;

    uart_enable_rx_interrupt(); // unmasks the interrupt line to get the next interrupt at the end of the task.
}

void uart_async_putc(char c)
{
    uart_tx_buffer[uart_tx_buffer_w_idx++] = c;
    uart_tx_buffer_w_idx %= MAX_BUF_SIZE;

    uart_enable_tx_interrupt();
}

void uart_async_puts(char *s)
{
    int i = 0;

    while (*s)
    {
        uart_async_putc(*s++);
        i++;
    }
    uart_async_putc('\r');
    uart_async_putc('\n');
}

char uart_async_getc()
{
    while (uart_rx_buffer_r_idx == uart_rx_buffer_w_idx)
        asm volatile("nop");

    char ret = uart_rx_buffer[uart_rx_buffer_r_idx++];
    uart_rx_buffer_r_idx %= MAX_BUF_SIZE;

    return ret;
}

char *uart_async_gets(char *buf)
{
    int count;
    char c;
    char *s;
    for (s = buf, count = 0; (c = uart_async_getc()) != '\n' && count != MAX_BUF_SIZE - 1; count++)
    {
        *s = c;
        if (*s == '\x7f')
        {
            count--;
            if (count == -1)
            {
                uart_async_putc(' '); // prevent back over command line #
                continue;
            }
            s--;
            count--;
            continue;
        }
        s++;
    }
    *s = '\0';
    return buf;
}

int uart_async_printf(char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[MAX_BUF_SIZE];
    char *s = (char *)buf;
    int count = vsprintf(s, fmt, args);
    while (*s)
    {
        if (*s == '\n')
            uart_async_putc('\r');
        uart_async_putc(*s++);
    }
    __builtin_va_end(args);
    return count;
}

/* For printf */
static unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args)
{
    long int arg;
    int len, sign, i;
    char *p, *orig = dst, tmpstr[19];

    // failsafes
    if (dst == (void *)0 || fmt == (void *)0)
    {
        return 0;
    }

    // main loop
    arg = 0;
    while (*fmt)
    {
        if (strlen(fmt) > 0x100 - 0x10 || dst - orig > 0x100 - 0x10)
        {
            *dst = 0;
            return dst - orig;
        }
        // argument access
        if (*fmt == '%')
        {
            fmt++;
            // literal %
            if (*fmt == '%')
            {
                goto put;
            }
            len = 0;
            // size modifier
            while (*fmt >= '0' && *fmt <= '9')
            {
                len *= 10;
                len += *fmt - '0';
                fmt++;
            }
            // skip long modifier
            if (*fmt == 'l')
            {
                fmt++;
            }
            // character
            if (*fmt == 'c')
            {
                arg = __builtin_va_arg(args, int);
                *dst++ = (char)arg;
                fmt++;
                continue;
            }
            else
                // decimal number
                if (*fmt == 'd')
                {
                    arg = __builtin_va_arg(args, int);
                    // check input
                    sign = 0;
                    if ((int)arg < 0)
                    {
                        arg *= -1;
                        sign++;
                    }
                    if (arg > 99999999999999999L)
                    {
                        arg = 99999999999999999L;
                    }
                    // convert to string
                    i = 18;
                    tmpstr[i] = 0;
                    do
                    {
                        tmpstr[--i] = '0' + (arg % 10);
                        arg /= 10;
                    } while (arg != 0 && i > 0);
                    if (sign)
                    {
                        tmpstr[--i] = '-';
                    }
                    // padding, only space
                    if (len > 0 && len < 18)
                    {
                        while (i > 18 - len)
                        {
                            tmpstr[--i] = ' ';
                        }
                    }
                    p = &tmpstr[i];
                    goto copystring;
                }
                else
                    // hex number
                    if (*fmt == 'x')
                    {
                        arg = __builtin_va_arg(args, long int);
                        // convert to string
                        i = 16;
                        tmpstr[i] = 0;
                        do
                        {
                            char n = arg & 0xf;
                            // 0-9 => '0'-'9', 10-15 => 'A'-'F'
                            tmpstr[--i] = n + (n > 9 ? 0x37 : 0x30);
                            arg >>= 4;
                        } while (arg != 0 && i > 0);
                        // padding, only leading zeros
                        if (len > 0 && len <= 16)
                        {
                            while (i > 16 - len)
                            {
                                tmpstr[--i] = '0';
                            }
                        }
                        p = &tmpstr[i];
                        goto copystring;
                    }
                    else
                        // string
                        if (*fmt == 's')
                        {
                            p = __builtin_va_arg(args, char *);
                        copystring:
                            if (p == (void *)0)
                            {
                                p = "(null)";
                            }
                            while (*p)
                            {
                                *dst++ = *p++;
                            }
                        }
        }
        else
        {
        put:
            *dst++ = *fmt;
        }
        fmt++;
    }
    *dst = 0;
    // number of bytes written
    return dst - orig;
}

/**
 * Variable length arguments
 */
static unsigned int sprintf(char *dst, char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    unsigned int r = vsprintf(dst, fmt, args);
    __builtin_va_end(args);
    return r;
}

