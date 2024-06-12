#ifndef MAILBOX_H
#define MAILBOX_H

#include <drivers/gpio.h>
#include <drivers/uart.h>

int mbox_call(unsigned char ch);
int get_board_revision(unsigned int *board_revision);
int get_arm_memory_info(unsigned int *base_addr, unsigned int *size);

#endif