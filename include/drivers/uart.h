#ifndef UART_H
#define UART_H

#include <drivers/mmio.h>
#include <drivers/gpio.h>
#include <kernel/exception.h>
#include <stddef.h>

#define MAX_BUF_SIZE    0x100

void uart_init();
void uart_enable_tx_interrupt();
void uart_disable_tx_interrupt();
void uart_enable_rx_interrupt();
void uart_disable_rx_interrupt();

/* Synchronous UART */
void uart_putc(char c);
void uart_puts(char *s);
char uart_getc();
char *uart_gets(char *buf);
int uart_printf(char *fmt, ...);

/* Asynchronous UART */
void uart_interrupt_handler();
static void uart_tx_interrupt_handler();
static void uart_rx_interrupt_handler();
void uart_async_putc(char c);
void uart_async_puts(char *s);
char uart_async_getc();
char *uart_async_gets(char *buf);
int uart_async_printf(char *fmt, ...);

/* util */
static unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args);
static unsigned int sprintf(char *dst, char *fmt, ...);
void uart_enable_echo();
void uart_disable_echo();

#endif