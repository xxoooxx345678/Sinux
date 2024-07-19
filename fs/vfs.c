#include <fs/vfs.h>
#include <fs/tmpfs.h>
#include <fs/initramfs.h>
#include <dev/console.h>
#include <dev/framebuffer.h>
#include <mm/mm.h>
#include <string.h>

#define MAX_FS_NUM 0x10
#define MAX_DEV_NUM 0x50

struct mount *rootfs;
struct filesystem reg_fs[MAX_FS_NUM];
struct file_operations reg_dev[MAX_FS_NUM];

static void get_parent_pathname(const char *pathname, char *target)
{
    strcpy(target, pathname);
    for (int i = strlen(pathname) - 1; i >= 0; --i)
    {
        target[i] = 0;
        if (pathname[i] == '/')
            break;
    }
}

static void get_file_or_dir_name(const char *pathname, char *target)
{
    int i;
    for (i = strlen(pathname) - 1; i >= 0; --i)
        if (pathname[i] == '/')
            break;
    int j = 0;
    while (pathname[i])
        target[j++] = pathname[++i];
    target[j] = 0;
}

static struct filesystem *find_filesystem(const char *fs_name)
{
    for (int i = 0; i < MAX_FS_NUM; i++)
    {
        if (strcmp(reg_fs[i].name, fs_name) == 0)
            return &reg_fs[i];
    }
    return 0;
}

void fs_init()
{
    int rootfs_idx = register_tmpfs();
    rootfs = malloc(sizeof(struct mount));
    reg_fs[rootfs_idx].setup_mount(&reg_fs[rootfs_idx], rootfs);

    vfs_mkdir("/initramfs");
    register_initramfs();
    vfs_mount("/initramfs", "initramfs");

    vfs_mkdir("/dev");
    int idx = register_dev_console();
    vfs_mknod("/dev/uart", &reg_dev[idx]);
    idx = register_dev_framebuffer();
    vfs_mknod("/dev/framebuffer", &reg_dev[idx]);
}

int register_filesystem(struct filesystem *fs)
{
    for (int i = 0; i < MAX_FS_NUM; ++i)
    {
        if (!reg_fs[i].name)
        {
            reg_fs[i].name = fs->name;
            reg_fs[i].setup_mount = fs->setup_mount;
            return i;
        }
    }
    return -1;
}

int register_dev(struct file_operations *fo)
{
    for (int i = 0; i < MAX_DEV_NUM; i++)
    {
        if (!reg_dev[i].open)
        {
            reg_dev[i] = *fo;
            return i;
        }
    }
    return -1;
}

int vfs_open(const char *pathname, int flags, struct file **target)
{
    struct vnode *vnode_ptr;

    if (vfs_lookup(pathname, &vnode_ptr) < 0) // vnode not found
    {
        if (flags & O_CREAT)
        {
            char pd_pathname[MAX_PATHNAME_LEN + 1];
            char file_name[MAX_PATHNAME_LEN + 1];
            get_parent_pathname(pathname, pd_pathname);
            get_file_or_dir_name(pathname, file_name);

            if (vfs_lookup(pd_pathname, &vnode_ptr) < 0)
                return -1;

            vnode_ptr->v_ops->create(vnode_ptr, &vnode_ptr, file_name);
        }
        else
            return -1;
    }

    // vnode found or created
    *target = malloc(sizeof(struct file));
    vnode_ptr->f_ops->open(vnode_ptr, target);
    (*target)->flags = flags;

    return 0;
}

int vfs_close(struct file *file)
{
    file->f_ops->close(file);
    return 0;
}

int vfs_write(struct file *file, const void *buf, size_t len)
{
    return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file *file, void *buf, size_t len)
{
    return file->f_ops->read(file, buf, len);
}

int vfs_mkdir(const char *pathname)
{
    char pd_pathname[MAX_PATHNAME_LEN + 1];
    char dir_name[MAX_PATHNAME_LEN + 1];

    get_parent_pathname(pathname, pd_pathname);
    get_file_or_dir_name(pathname, dir_name);

    struct vnode *vnode_ptr;

    if (vfs_lookup(pd_pathname, &vnode_ptr) == 0)
    {
        vnode_ptr->v_ops->mkdir(vnode_ptr, &vnode_ptr, dir_name);
        return 0;
    }

    return -1;
}

int vfs_mount(const char *target, const char *filesystem)
{
    struct vnode *vnode_ptr;
    struct filesystem *fs = find_filesystem(filesystem);

    if (vfs_lookup(target, &vnode_ptr) < 0)
        return -1;

    vnode_ptr->mount = malloc(sizeof(struct mount));
    vnode_ptr->mount->fs = fs;
    fs->setup_mount(fs, vnode_ptr->mount);
}

int vfs_lookup(const char *pathname, struct vnode **target)
{
    if (strlen(pathname) == 0)
    {
        *target = rootfs->root;
        return 0;
    }

    struct vnode *vnode_ptr = rootfs->root;

    char component_name[MAX_PATHNAME_LEN + 1];
    strcpy(component_name, pathname);
    char *delim = "/";
    char *p = strtok(component_name, delim);

    while (p)
    {
        if (vnode_ptr->v_ops->lookup(vnode_ptr, &vnode_ptr, p) != 0)
            return -1;
        
        /* Redirect to another filesystem (why not if ?) */
        while (vnode_ptr->mount)
            vnode_ptr = vnode_ptr->mount->root;

        p = strtok(NULL, delim);
    }

    *target = vnode_ptr;

    return 0;
}

long vfs_lseek64(struct file *file, long offset, int whence)
{
    struct vnode *vnode_ptr = file->vnode;
    return vnode_ptr->f_ops->lseek64(file, offset, whence);
}

int vfs_mknod(const char *pathname, struct file_operations *fo)
{
    struct file *f = malloc(sizeof(struct file));

    vfs_open(pathname, O_CREAT, &f);
    f->vnode->f_ops = fo;
    vfs_close(f);
    return 0;
}

void resolve_path(const char *pathname, const char *cwd, char *target)
{
    // relative
    char tmp[MAX_PATHNAME_LEN + 1];
    if (pathname[0] != '/')
    {
        strcpy(tmp, cwd);
        if (strcmp(cwd, "/") != 0)
            strcat(tmp, "/");
        strcat(tmp, pathname);
    }
    else
        strcpy(tmp, pathname);

    char absolute_path[MAX_PATHNAME_LEN + 1] = {0};
    int idx = 0;
    for (int i = 0; i < strlen(tmp); i++)
    {
        // meet /..
        if (tmp[i] == '/' && tmp[i + 1] == '.' && tmp[i + 2] == '.')
        {
            for (int j = idx; j >= 0; j--)
            {
                if (absolute_path[j] == '/')
                {
                    absolute_path[j] = 0;
                    idx = j;
                    break;
                }
            }
            i += 2;
            continue;
        }

        // ignore /.
        if (tmp[i] == '/' && tmp[i + 1] == '.')
        {
            i++;
            continue;
        }

        absolute_path[idx++] = tmp[i];
    }

    absolute_path[idx] = 0;

    strcpy(target, absolute_path);
}