#ifndef TMPFS_H
#define TMPFS_H

#include <fs/vfs.h>
#include <stddef.h>

#define TMPFS_MAX_FILE_NAME_LEN 16
#define TMPFS_MAX_DIR_ENTRY 16

struct tmpfs_inode 
{
    node_type type;
    char name[TMPFS_MAX_FILE_NAME_LEN];
    struct vnode *entry[TMPFS_MAX_DIR_ENTRY];
    char *data;
    size_t datasize;
};

int register_tmpfs();
int tmpfs_setup_mount(struct filesystem *fs, struct mount *_mount);

/* file operations */
int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);
long tmpfs_lseek64(struct file *file, long offset, int whence);
long tmpfs_getsize(struct vnode *file_node);

/* vnode operations */
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

struct vnode *tmpfs_create_vnode(struct mount *_mount, node_type type);

#endif