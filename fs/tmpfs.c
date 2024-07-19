#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <mm/mm.h>
#include <string.h>

struct file_operations tmpfs_file_operations = {tmpfs_write, tmpfs_read, tmpfs_open, tmpfs_close, tmpfs_lseek64, tmpfs_getsize};
struct vnode_operations tmpfs_vnode_operations = {tmpfs_lookup, tmpfs_create, tmpfs_mkdir};

int register_tmpfs()
{
    struct filesystem fs;
    fs.name = "tmpfs";
    fs.setup_mount = tmpfs_setup_mount;

    return register_filesystem(&fs);
}

int tmpfs_setup_mount(struct filesystem *fs, struct mount *_mount)
{
    _mount->fs = fs;
    _mount->root = tmpfs_create_vnode(0, DIRECTORY);
    return 0;
}

int tmpfs_write(struct file *file, const void *buf, size_t len)
{
    struct tmpfs_inode *inode_ptr = file->vnode->internal;

    memcpy(inode_ptr->data + file->f_pos, buf, len);
    file->f_pos += len;
    inode_ptr->datasize = inode_ptr->datasize < file->f_pos ? file->f_pos : inode_ptr->datasize;

    return len;
}  

int tmpfs_read(struct file *file, void *buf, size_t len)
{
    struct tmpfs_inode *inode_ptr = file->vnode->internal;

    len = (file->f_pos + len < inode_ptr->datasize ? len : inode_ptr->datasize - file->f_pos);
    memcpy(buf, inode_ptr->data + file->f_pos, len);
    file->f_pos += len;

    return len;
}

int tmpfs_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 0;
}

int tmpfs_close(struct file *file)
{
    free(file); 
    return 0;
}

long tmpfs_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET)
    {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

long tmpfs_getsize(struct vnode *file_node)
{
    struct tmpfs_inode *inode_ptr = file_node->internal;
    return inode_ptr->datasize;
}

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode_ptr = dir_node->internal;
    
    for (int i = 0; i < TMPFS_MAX_DIR_ENTRY; ++i)
    {
        struct vnode *vnode_ptr = inode_ptr->entry[i];

        if (!vnode_ptr)
            return -1;
        
        struct tmpfs_inode *inode_ptr = vnode_ptr->internal;
        if (strcmp(component_name, inode_ptr->name) == 0)
        {
            *target = vnode_ptr;
            return 0;
        }
    }

    return -1;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode_ptr = dir_node->internal;

    if (inode_ptr->type != DIRECTORY)
        return -1;

    int child_idx;
    for (child_idx = 0; child_idx < TMPFS_MAX_DIR_ENTRY; ++child_idx)
    {
        if (inode_ptr->entry[child_idx] == NULL)
            break;

        struct tmpfs_inode *child_inode_ptr = inode_ptr->entry[child_idx]->internal;
        if (strcmp(child_inode_ptr->name, component_name) == 0)
            return -1;
    }

    if (child_idx == TMPFS_MAX_DIR_ENTRY)
        return -1;
    
    struct vnode *vnode_ptr = tmpfs_create_vnode(0, FILE);
    inode_ptr->entry[child_idx] = vnode_ptr;
    
    struct tmpfs_inode *new_inode_ptr = vnode_ptr->internal;
    strcpy(new_inode_ptr->name, component_name);
    new_inode_ptr->data = malloc(PAGE_FRAME_SIZE);

    *target = vnode_ptr;
    return 0;
}

int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode_ptr = dir_node->internal;

    if (inode_ptr->type != DIRECTORY)
        return -1;

    int child_idx;
    for (child_idx = 0; child_idx < TMPFS_MAX_DIR_ENTRY; ++child_idx)
    {
        if (inode_ptr->entry[child_idx] == NULL)
            break;
        
        struct tmpfs_inode *child_inode_ptr = inode_ptr->entry[child_idx]->internal;
        if (strcmp(child_inode_ptr->name, component_name) == 0)
            return -1;
    }

    if (child_idx == TMPFS_MAX_DIR_ENTRY)
        return -1;
    
    struct vnode *vnode_ptr = tmpfs_create_vnode(0, DIRECTORY);
    inode_ptr->entry[child_idx] = vnode_ptr;
    
    struct tmpfs_inode *new_inode_ptr = vnode_ptr->internal;
    strcpy(new_inode_ptr->name, component_name);

    *target = vnode_ptr;
    return 0;
}

struct vnode *tmpfs_create_vnode(struct mount *_mount, node_type type)
{
    struct vnode *vnode_ptr = malloc(sizeof(struct vnode));

    vnode_ptr->mount = NULL;
    vnode_ptr->f_ops = &tmpfs_file_operations;
    vnode_ptr->v_ops = &tmpfs_vnode_operations;

    struct tmpfs_inode *inode_ptr = malloc(sizeof(struct tmpfs_inode));
    memset(inode_ptr, 0, sizeof(struct tmpfs_inode));
    inode_ptr->type = type;
    inode_ptr->data = NULL;

    vnode_ptr->internal = inode_ptr;
    return vnode_ptr;
}