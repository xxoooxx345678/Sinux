#include <fs/initramfs.h>
#include <fs/vfs.h>
#include <fs/cpio.h>
#include <mm/mm.h>
#include <string.h>

struct file_operations initramfs_file_operations = {initramfs_write, initramfs_read, initramfs_open, initramfs_close, initramfs_lseek64, initramfs_getsize};
struct vnode_operations initramfs_vnode_operations = {initramfs_lookup, initramfs_create, initramfs_mkdir};

extern char *cpio_start;

int register_initramfs()
{
    struct filesystem fs;
    fs.name = "initramfs";
    fs.setup_mount = initramfs_setup_mount;

    return register_filesystem(&fs);
}

int initramfs_setup_mount(struct filesystem *fs, struct mount *_mount)
{
    _mount->fs = fs;
    _mount->root = initramfs_create_vnode(0, DIRECTORY);
    struct initramfs_inode *inode_ptr = _mount->root->internal;

    char *filepath;
    char *filedata;
    unsigned int filesize;
    int child_idx = 0;
    struct cpio_newc_header *header_pointer = (struct cpio_newc_header *)cpio_start;

    while (header_pointer)
    {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error)
        {
            uart_printf("error\n");
            break;
        }

        // if this is not TRAILER!!! (last of file)
        if (header_pointer != 0)
        {
            struct vnode *new_vnode_ptr = initramfs_create_vnode(0, FILE);
            struct initramfs_inode *new_inode_ptr = new_vnode_ptr->internal;
            new_inode_ptr->data = filedata;
            new_inode_ptr->datasize = filesize;
            strcpy(new_inode_ptr->name, filepath);

            inode_ptr->entry[child_idx++] = new_vnode_ptr; 
        }
    }
}

/* file operations */
int initramfs_write(struct file *file, const void *buf, size_t len)
{
    return -1;
}

int initramfs_read(struct file *file, void *buf, size_t len)
{
    struct initramfs_inode *inode_ptr = file->vnode->internal;

    len = (file->f_pos + len < inode_ptr->datasize ? len : inode_ptr->datasize - file->f_pos);
    memcpy(buf, inode_ptr->data + file->f_pos, len);
    file->f_pos += len;

    return len;
}

int initramfs_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 0;
}

int initramfs_close(struct file *file)
{
    free(file);
    return 0;
}

long initramfs_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET)
    {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

long initramfs_getsize(struct vnode *file_node)
{
    struct initramfs_inode *inode_ptr = file_node->internal;
    return inode_ptr->datasize;
}

/* vnode operations */
int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct initramfs_inode *inode_ptr = dir_node->internal;
    
    for (int i = 0; i < INITRAMFS_MAX_DIR_ENTRY; ++i)
    {
        struct vnode *vnode_ptr = inode_ptr->entry[i];

        if (!vnode_ptr)
            return -1;
        
        struct initramfs_inode *inode_ptr = vnode_ptr->internal;
        if (strcmp(component_name, inode_ptr->name) == 0)
        {
            *target = vnode_ptr;
            return 0;
        }
    }

    return -1;
}

int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    return -1;
}

int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    return -1;
}

struct vnode *initramfs_create_vnode(struct mount *_mount, node_type type)
{
    struct vnode *vnode_ptr = malloc(sizeof(struct vnode));

    vnode_ptr->mount = NULL;
    vnode_ptr->f_ops = &initramfs_file_operations;
    vnode_ptr->v_ops = &initramfs_vnode_operations;

    struct initramfs_inode *inode_ptr = malloc(sizeof(struct initramfs_inode));
    memset(inode_ptr, 0, sizeof(struct initramfs_inode));
    inode_ptr->type = type;
    inode_ptr->data = NULL;

    vnode_ptr->internal = inode_ptr;
    return vnode_ptr;
}