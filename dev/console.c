#include <dev/console.h>
#include <mm/mm.h>
#include <fs/vfs.h>

struct file_operations dev_console_file_operations = {dev_console_write, dev_console_read, dev_console_open, dev_console_close, dev_console_lseek64, dev_console_getsize};

int register_dev_console()
{
    return register_dev(&dev_console_file_operations);
}

int dev_console_write(struct file *file, const void *buf, size_t len)
{
    const char *_buf = buf;
    int i;
    for (i = 0; i < len; ++i)
        uart_async_putc(_buf[i]);

    return i;
}

int dev_console_read(struct file *file, void *buf, size_t len)
{
    char *_buf = buf;
    int i;
    for (i = 0; i < len; ++i)
        _buf[i] = uart_async_getc();

    return len;
}

int dev_console_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_console_file_operations;
    return 0;
}

int dev_console_close(struct file *file)
{
    free(file);
    return 0;
}
long dev_console_lseek64(struct file *file, long offset, int whence)
{
    return -1;
}

long dev_console_getsize(struct vnode *file_node)
{
    return -1;
}