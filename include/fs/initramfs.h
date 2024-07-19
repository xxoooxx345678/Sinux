#ifndef INITRAMFS_H
#define INITRAMFS_H

#include <fs/vfs.h>
#include <stddef.h>

#define INITRAMFS_MAX_FILE_NAME_LEN 256
#define INITRAMFS_MAX_DIR_ENTRY 128

struct initramfs_inode 
{
    node_type type;
    char name[INITRAMFS_MAX_FILE_NAME_LEN];
    struct vnode *entry[INITRAMFS_MAX_DIR_ENTRY];
    char *data;
    size_t datasize;
};

int register_initramfs();
int initramfs_setup_mount(struct filesystem *fs, struct mount *_mount);

/* file operations */
int initramfs_write(struct file *file, const void *buf, size_t len);
int initramfs_read(struct file *file, void *buf, size_t len);
int initramfs_open(struct vnode *file_node, struct file **target);
int initramfs_close(struct file *file);
long initramfs_lseek64(struct file *file, long offset, int whence);
long initramfs_getsize(struct vnode *file_node);

/* vnode operations */
int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

struct vnode *initramfs_create_vnode(struct mount *_mount, node_type type);

#endif