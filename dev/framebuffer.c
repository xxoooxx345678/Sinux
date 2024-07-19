#include <dev/framebuffer.h>
#include <fs/vfs.h>
#include <stddef.h>
#include <drivers/mailbox.h>

struct file_operations dev_framebuffer_operations = {dev_framebuffer_write, dev_framebuffer_read, dev_framebuffer_open, dev_framebuffer_close, dev_framebuffer_lseek64, dev_framebuffer_getsize};

unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
unsigned char *lfb;                       /* raw frame buffer address */

int register_dev_framebuffer()
{
    init_framebuffer();
    return register_dev(&dev_framebuffer_operations);
}

int dev_framebuffer_write(struct file *file, const void *buf, size_t len)
{
    len = (len + file->f_pos > pitch * height) ? pitch * height - file->f_pos : len;

    memcpy(lfb + file->f_pos, buf, len);
    file->f_pos += len;
    return len;
}

int dev_framebuffer_read(struct file *file, void *buf, size_t len)
{
    return -1;
}

int dev_framebuffer_open(struct vnode *file_node, struct file **target)
{
    (*target)->f_pos = 0;
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_framebuffer_operations;
    return 0;
}

int dev_framebuffer_close(struct file *file)
{
    free(file);
    return 0;
}

long dev_framebuffer_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET)
    {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

long dev_framebuffer_getsize(struct vnode *file_node)
{
    return -1;
}