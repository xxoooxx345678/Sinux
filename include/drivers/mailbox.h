#ifndef MAILBOX_H
#define MAILBOX_H

int mbox_call(unsigned char ch, unsigned int *mbox);
int get_board_revision(unsigned int *board_revision);
int get_arm_memory_info(unsigned int *base_addr, unsigned int *size);
void init_framebuffer();

#endif