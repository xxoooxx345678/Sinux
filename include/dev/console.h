#ifndef CONSOLE_H
#define CONSOLE_H

#include <fs/vfs.h>
#include <stddef.h>

int register_dev_console();

int dev_console_write(struct file *file, const void *buf, size_t len);
int dev_console_read(struct file *file, void *buf, size_t len);
int dev_console_open(struct vnode *file_node, struct file **target);
int dev_console_close(struct file *file);
long dev_console_lseek64(struct file *file, long offset, int whence);
long dev_console_getsize(struct vnode *file_node);


#endif