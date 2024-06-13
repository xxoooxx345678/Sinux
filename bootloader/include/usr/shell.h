#ifndef SHELL_H
#define SHELL_H

#include <drivers/uart.h>
#include <string.h>
#include <stddef.h>
#include <load.h>

void shell();
void cmd_resolve(char *cmd);
void clear();
void print_boot_msg();

#endif