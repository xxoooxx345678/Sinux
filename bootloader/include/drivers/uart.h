#ifndef UART_H
#define UART_H

#define MAX_BUF_SIZE    0x100

void uart_init();

/* Synchronous UART */
void uart_putc(char c);
void uart_puts(char *s);
char uart_raw_getc();
char uart_getc();
char *uart_gets(char *buf);
int uart_printf(char *fmt, ...);

/* util */
static unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args);
static unsigned int sprintf(char *dst, char *fmt, ...);

#endif