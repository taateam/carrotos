#ifndef SYSCALL1_H
#define SYSCALL1_H

/*
#define SYSCALL_PRINT 1
#define SYSCALL_HLT 2
*/

#define SEEK_SET 0 
#define SEEK_CUR 1 
#define SEEK_END 2 


#define SEEK_DATA 3 
#define SEEK_HOLE 4 

#include <stdint.h>
#include <stddef.h>
// size 80;
/* struct file_ops
{
    uint64_t (*read)(uint64_t fd, uint64_t buf, uint64_t count);
    uint64_t (*write)(uint64_t fd, uint64_t buf, uint64_t count);
    uint64_t (*ioctl)(uint64_t fd, uint64_t request, uint64_t arg);
};*/
void print_to_console(char *msg, uint64_t len)
{
    echo(msg); 
}

// void halt_current_thread()
// {
//     uint64_t tid;
//     __asm__ volatile("mov %%gs:8, %0" : "=r"(tid));
//     thread_info_t *thread = threads_info_ptr[tid];

//     uint64_t sleep_ticks;
//     __asm__ volatile("mov %%rbx, %0" : "=r"(sleep_ticks));

// }
struct file_descriptor_inner
{
    int fd;               
    uint64_t fd_global_i; 
    uint64_t flags;       
    uint64_t ref_count;   
};
uint64_t sys_read_file(uint64_t fd, uint8_t *buf, uint64_t count)
{
    process_info_t *proc = processes_info_ptr[get_current_pid()];
    struct file_inner *file = &((struct file_inner *)proc->fd_addr)[fd];
    uint64_t start_pos = file->offset;
    uint64_t actual_read_len = 0;
    actual_read_len = read_file(fd_global[file->global_id]->path, buf, start_pos, count);
    file->offset += actual_read_len;
    fd_global[file->global_id]->offset += actual_read_len;
    return actual_read_len;
}
uint64_t sys_read_dir(uint64_t fd, uint8_t *buf, uint64_t count)
{
    process_info_t *proc = processes_info_ptr[get_current_pid()];
    struct file_inner *file = &((struct file_inner *)proc->fd_addr)[fd];
    uint64_t start_pos = file->offset;
    uint64_t actual_read_len = 0;
    actual_read_len = list_dir(fd_global[file->global_id]->path, buf, start_pos, count);
    file->offset += actual_read_len;
    fd_global[file->global_id]->offset += actual_read_len;
    return actual_read_len;
}
uint64_t sys_read_socket(uint64_t socket_id, uint8_t *buf, uint64_t count)
{
    uint64_t actual_read_len = read_socket_by_id(socket_id, buf, count);
    // file->offset += actual_read_len;
    // fd_global[file->global_id]->offset += actual_read_len;
    return actual_read_len;
}
uint64_t sys_read_socket_uds(uint64_t socket_id, uint8_t *buf, uint64_t count)
{
    uint64_t actual_read_len = read_socket_by_id_uds(socket_id, buf, count);
    // file->offset += actual_read_len;
    // fd_global[file->global_id]->offset += actual_read_len;
    return actual_read_len;
}
uint64_t sys_read_tty(uint64_t tty_id, struct input_key *buf, uint64_t count, bool non_blocking)
{
    struct tty *tty = tty_global[tty_id];
    if (!tty)
        return (uint64_t)-EBADF;

    uint64_t bytes_read = 0;

    if (!non_blocking)
        bytes_read = tty_read_key_no_halt(tty, buf, count);
    else
        bytes_read = tty_read_key_halt(tty, buf, count);
    return bytes_read;
}
uint64_t sys_read_pipe(uint64_t pipe_id, uint8_t *buf, uint64_t start_pos, uint64_t count, bool non_blocking)
{
    struct pipe *p = pipe_global[pipe_id];
    if (!p)
        return (uint64_t)-EBADF;

    uint64_t bytes_read = fifo_read8(p->buf + start_pos, &p->tail, &p->head, PIPE_BUF_SIZE, count, buf);

    return bytes_read;
}
uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count)
{
    struct file_inner *file = (struct file_inner *)&((struct file_inner *)processes_info_ptr[get_current_pid()]->fd_addr)[fd];
    // if (!file)
    //     return -1;
    if ((file->open_flags != O_RDONLY) /*not read only*/ && !(file->open_flags & O_RDWR))
        return -EBADF;

    uint64_t start_pos = file->offset;
    bool non_blocking = !(file->open_flags & O_NONBLOCK);

    switch (file->type)
    {
    case FILE_NONE:
        return -1;
    case FILE_REGULAR:
        return sys_read_file(fd, (uint8_t *)buf, count);
    case FILE_CHAR_DEV:
        return 0; // TODO: handle later;
    case FILE_DIR:
        return sys_read_dir(fd, (uint8_t *)buf, count);
    case FILE_PIPE:
        return sys_read_pipe(file->res_id, (uint8_t *)buf, start_pos, count, non_blocking);
    case FILE_SOCKET:
        return sys_read_socket(file->res_id, (uint8_t *)buf, count);
    case FILE_SOCKET_UDS:
        return sys_read_socket_uds(file->res_id, (uint8_t *)buf, count);
    case FILE_TTY:
        return sys_read_tty(file->res_id, (struct input_key *)buf, count, non_blocking);
    case FILE_FRAME_BUFFER:
        return sys_read_frame_buffer(file->res_id, (uint8_t *)buf, count);
    case FILE_SPECIAL:
        return sys_read_special_file(file->res_id, (uint8_t *)buf, count);
    };
}
uint64_t sys_write_file(uint64_t fd, uint8_t *buf, uint64_t count)
{
    uint64_t pid = get_current_pid();
    process_info_t **tmp1 = processes_info_ptr;
    struct file_inner *fi = (struct file_inner *)tmp1[pid]->fd_addr;
    uint64_t fd_global_i = fi->global_id;
    struct file *fdg = fd_global[fd_global_i];
    uint64_t start_pos = fi->offset;
    uint64_t actual_written_len = write_file(fdg->path, buf, start_pos, count);
    fi->offset += actual_written_len;
    fdg->offset += actual_written_len;
    return actual_written_len;
}
uint64_t sys_write_pipe(uint64_t id, uint8_t *buf, uint64_t count)
{
    struct pipe *tmp = pipe_global[id];
    return fifo_write8(tmp->buf, &tmp->tail, &tmp->head, PIPE_BUF_SIZE, count, (uint8_t *)buf);
}
uint64_t sys_write_socket(uint64_t id, uint8_t *buf, uint64_t count)
{
    // struct net_tuple *tmp = sockets_global[id];
    return 0; //  udp_socket_append(id, buf, count);
}
uint64_t sys_write_socket_uds(uint64_t id, uint8_t *buf, uint64_t count)
{
    // struct net_tuple *tmp = sockets_global[id];
    return socket_append_uds(id, buf, count);
}
uint64_t sys_write_tty(uint64_t id, uint8_t *buf, uint64_t count)
{
    struct tty *tmp = tty_global[id];
    return tty_write_output(tmp, (struct input_key *)buf, count);
}
uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count)
{
    
    if (buf == 0 || count == 0)
        return 0;

    uint64_t pid = get_current_pid();
    struct file_inner *fd_ptr = (struct file_inner *)processes_info_ptr[pid]->fd_addr;
    struct file_inner *file = (struct file_inner *)&fd_ptr[fd];
    // if (!file)
    //     return -1;
    if (!(file->open_flags & O_WRONLY) && !(file->open_flags & O_RDWR))
        return -EBADF; 

    uint64_t start_pos = file->offset;
    bool blocking = !(file->open_flags & O_NONBLOCK);

    switch (file->type)
    {
    case FILE_NONE:
        return -1;
    case FILE_REGULAR:
        return sys_write_file(fd, (uint8_t *)buf, count);
    case FILE_CHAR_DEV:
        return 0; // TODO: handle later;
    case FILE_DIR:
        return -EISDIR;
    case FILE_PIPE:
        return sys_write_pipe(file->res_id, (uint8_t *)buf, count);
    case FILE_SOCKET:
        return sys_write_socket(file->res_id, (uint8_t *)buf, count);
    case FILE_SOCKET_UDS:
        return sys_write_socket_uds(file->res_id, (uint8_t *)buf, count);
    case FILE_TTY:
        return sys_write_tty(file->res_id, (uint8_t *)buf, count);
    case FILE_FRAME_BUFFER:
        return sys_write_frame_buffer(file->res_id, (uint8_t *)buf, count);
    case FILE_SPECIAL:
        return sys_write_special_file(file->res_id, (uint8_t *)buf, count);
    }
}
uint64_t extract_id_from_path(const char *path)
{
    int i = strlen(path) - 1;
    
    uint64_t id = 0;
    uint64_t base = 1;
    while (i >= 0 && path[i] >= '0' && path[i] <= '9')
    {
        id += (path[i] - '0') * base;
        base *= 10;
        i--;
    }
    return id;
}
uint64_t sys_open_tty(char *path, int flags, int mode)
{
    uint64_t pid = get_current_pid();
    process_info_t **processes = processes_info_ptr;
    uint64_t fd_addr = processes[pid]->fd_addr;

    
    uint64_t fd_id_inner = get_a_free_slot_with_size(fd_addr, 0, MAX_FD_INNER, sizeof(struct file_inner));
    struct file_inner *file_inner_tmp = &((struct file_inner *)fd_addr)[fd_id_inner];

    
    uint64_t new_block_addr = get_a_free_block_addr();
    uint64_t fd_id_global = get_a_free_slot((uint64_t)fd_global, 0, MAX_FD);
    fd_global[fd_id_global] = (struct file *)new_block_addr;
    struct file *file_global_tmp = (struct file *)new_block_addr;

    file_inner_tmp->global_id = fd_id_global;
    // 3. Copy path
    uint64_t path_len = strlen(path);
    if (path_len >= MAX_PATH)
        path_len = MAX_PATH - 1;
    // memcpy(file_inner_tmp->path, path, path_len);
    // file_inner_tmp->path[path_len] = 0;
    memcpy(file_global_tmp->path, path, path_len);
    // 4. others
    file_inner_tmp->open_flags;
    file_inner_tmp->offset = 0;
    file_inner_tmp->open_flags = flags;
    file_inner_tmp->type = FILE_TTY;
    file_global_tmp->offset = 0;
    file_global_tmp->open_flags = flags;
    file_global_tmp->type = FILE_TTY;

    file_inner_tmp->global_id = fd_id_global;
    file_global_tmp->id = fd_id_global;
    return fd_id_inner;
}
uint64_t sys_open_dirent(char *path, int flags, int mode)
{
    enum file_type ft;
    struct DirentStat stat;
    stat_dirent(path, &stat);
    // create if file not exists;
    enum PathType type = stat.type;
    bool flag_truncate = (flags & O_TRUNC) > 0;
    bool flag_create = (flags & O_CREAT) > 0;
    bool flag_writeonly = (flags & O_WRONLY) > 0;
    bool flag_readwrite = (flags & O_RDWR) > 0;
    bool flag_excl = (flags & O_EXCL) > 0;
    //======================================
    if (flag_readwrite)
    {
        if (!allow_write_path(path) || !allow_read_path(path))
        {
            return -EACCES;
        }
    }
    else if (flag_create || flag_truncate || flag_writeonly)
    {
        if (!allow_write_path(path))
        {
            return -EACCES;
        }
    }
    else
    {
        if (!allow_read_path(path))
        {
            return -EACCES;
        }
    }
    //========================================
    if (type == PATH_DIRECTORY)
    {
        if (flag_create || flag_truncate || flag_writeonly || flag_readwrite)
        {
            return -EISDIR; 
        }
        ft = FILE_DIR;
    }
    else if (type == PATH_NOT_FOUND)
    {
        ft = FILE_REGULAR;
        if (flag_create)
        {
            uint64_t result = mkfile(path);
            if (result != 0)
                return result;
        }
        else
            return -ENOENT;
    }
    else // if (type == PATH_FILE);
    {
        if (flag_create && flag_excl)
        {
            return -EEXIST;
        }
        if (flag_truncate)
        {
            erase_file(path);
        }
        ft = FILE_REGULAR;
    }
    uint64_t pid = get_current_pid();
    process_info_t **processes = processes_info_ptr;
    uint64_t fd_addr = processes[pid]->fd_addr;

    
    uint64_t fd_id_inner = get_a_free_slot_with_size(fd_addr, 0, MAX_FD_INNER, sizeof(struct file_inner));
    struct file_inner *file_inner_tmp = &((struct file_inner *)fd_addr)[fd_id_inner];

    
    uint64_t new_block_addr = get_a_free_block_addr();
    uint64_t fd_id_global = get_a_free_slot((uint64_t)fd_global, 0, MAX_FD);
    fd_global[fd_id_global] = (struct file *)new_block_addr;
    struct file *file_global_tmp = (struct file *)new_block_addr;

    file_inner_tmp->global_id = fd_id_global;
    // 3. Copy path
    uint64_t path_len = strlen(path);
    if (path_len >= MAX_PATH)
        path_len = MAX_PATH - 1;
    // memcpy(file_inner_tmp->path, path, path_len);
    // file_inner_tmp->path[path_len] = 0;
    memcpy(file_global_tmp->path, path, path_len);
    file_global_tmp->path[path_len] = 0;

    file_inner_tmp->offset = 0;
    file_inner_tmp->open_flags = flags;
    file_inner_tmp->type = ft;
    file_global_tmp->offset = 0;
    file_global_tmp->open_flags = flags;
    file_global_tmp->type = ft;

    file_inner_tmp->global_id = fd_id_global;
    file_global_tmp->id = fd_id_global;

    return fd_id_inner;
}
uint64_t sys_open(char *path, int flags, int mode)
{
    if (!path)
        return -1;
    // tty
    if (memcmp(path, "/dev/tty", 8) == 0)
    {
        return sys_open_tty(path, flags, mode);
    }
    if (is_special_dirent(path))
    {
        return sys_open_special_dirent(path, flags, mode);
    }
    // file
    return sys_open_dirent(path, flags, mode);
}
int64_t sys_close(int fd_inner_id)
{
    if (fd_inner_id < 0)
        return -EINVAL;
    uint64_t pid = get_current_pid();
    process_info_t **processes = processes_info_ptr;
    uint64_t fd_addr = processes[pid]->fd_addr;
    struct file_inner *fd_ptr = (struct file_inner *)fd_addr;
    if (!fd_ptr)
        return (uint64_t)-EBADF;
    uint64_t fd_id_global = fd_ptr->global_id;
    erase_mem8(fd_addr, 4096);
    set_memory_block_used_when_boot((uint64_t)fd_ptr / 4096, false);
    *((uint64_t *)fd_addr + 8 * fd_inner_id) = 0;

    struct file *file_global_tmp = fd_global[fd_id_global];
    erase_mem8((uint64_t)file_global_tmp, 4096);
    set_memory_block_used_when_boot((uint64_t)file_global_tmp / 4096, false);
    *((uint64_t *)fd_global + 8 * fd_id_global) = 0;

    return 0;
}

enum
{
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_OPEN = 2,
    SYS_CLOSE = 3,
    SYS_SEEK = 4,
};
int64_t sys_seek(int fd, int64_t offset, int whence)
{
    if (fd < 0 || fd >= MAX_FD)
        return -1;

    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    struct file_inner *desc = (struct file_inner *)tmp[pid]->fd_addr + 8 * fd;
    if (((uint64_t)desc) != 0)
        return -1;

    struct file *f = (struct file *)desc;
    int64_t new_pos;

    switch (whence)
    {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = (int64_t)f->offset + offset;
        break;
    case SEEK_END:
        new_pos = (int64_t)f->meta.size + offset;
        break;
    default:
        return -1;
    }

    // check out of range
    if (new_pos < 0)
        new_pos = 0;
    if ((uint64_t)new_pos > f->meta.size)
        new_pos = f->meta.size;

    f->offset = (uint64_t)new_pos;
    return f->offset;
}
int64_t sys_mkdir(char *path, int mode)
{
    return mkdir(path);
}
int64_t sys_unlink(char *path, int mode)
{
    return delete_dirent(path);
}
int64_t sys_rename(char *path_src, char *path_dst)
{
    if (!path_src || !path_dst)
        return -1;
    return rename_dirent(path_src, path_dst);
}
struct dirent_stat
{
    uint64_t st_dev;   
    uint64_t st_ino;   
    uint32_t st_mode;  
    uint32_t st_nlink; 
    uint32_t st_uid;   
    uint32_t st_gid;   
    uint64_t st_size;  
    uint64_t st_atime; // last access time (epoch)
    uint64_t st_mtime; // last modification time
    uint64_t st_ctime; // last status change time
} __attribute__((packed));
int64_t sys_stat(char *path, struct dirent_stat *stat)
{
    if (!path || !stat)
        return -1;

    struct DirentStat real_stat;
    if (!path_exists(path))
        return -1; 

    
    stat->st_dev = 0; 
    stat->st_ino = 0; 
    stat->st_mode = (real_stat.type == PATH_DIRECTORY ? 0x4000 : 0x8000) | 0777;
    
    stat->st_nlink = 1; 
    stat->st_uid = 0;   // root user
    stat->st_gid = 0;   // root group
    stat->st_size = real_stat.size;

    
    stat->st_atime = (uint64_t)real_stat.last_access_date * 24ULL * 3600ULL + (uint64_t)real_stat.last_access_time;
    stat->st_mtime = (uint64_t)real_stat.modified_date * 24ULL * 3600ULL + (uint64_t)real_stat.modified_time;
    stat->st_ctime = (uint64_t)real_stat.creation_date * 24ULL * 3600ULL + (uint64_t)real_stat.creation_time;

    return 0;
}

struct pollfd
{
    int fd;        // file descriptor
    short events;  // events to monitor
    short revents; // events returned
};
int64_t sys_poll(struct pollfd *fds, uint64_t nfds, int64_t timeout)
{
    
    for (uint64_t i = 0; i < nfds; i++)
    {
        fds[i].revents = fds[i].events; 
    }

    
    return (int64_t)nfds;
}

//===========================================================================


uint64_t sys_ioctl_file(uint64_t global_id, uint64_t request, uint64_t arg)
{
    return 0;
}

// dummy ioctl cho pipe
uint64_t sys_ioctl_pipe(uint64_t res_id, uint64_t request, uint64_t arg)
{
    return 0;
}

// dummy ioctl cho socket
uint64_t sys_ioctl_socket(uint64_t res_id, uint64_t request, uint64_t arg)
{
    return 0;
}

#define TTY_IOCTL_ASSIGN_FOREGROUND 0x2001
struct tty_assign_fg_args
{
    uint64_t pid; 
};
uint64_t sys_ioctl_tty(uint64_t tty_id, uint64_t request, void *arg)
{
    struct tty *tty = tty_global[tty_id];
    if (!tty)
        return -ENODEV;

    switch (request)
    {
    case TTY_IOCTL_ASSIGN_FOREGROUND:
    {
        if (!arg)
            return -EINVAL;

        struct tty_assign_fg_args *args = (struct tty_assign_fg_args *)arg;

        return assign_tty(args->pid, tty_id);
    }

    default:
        return 0; // ioctl dummy;
    }
}

#define FBIO_RESIZE 0x1001

struct fb_resize_args
{
    uint32_t new_width;
    uint32_t new_height;
    uint8_t new_bpp;
};

uint64_t sys_ioctl_frame_buffer(uint64_t fb_id, uint64_t request, void *arg)
{
    if (!fb_global[fb_id])
        return -ENOENT;

    switch (request)
    {
    case FBIO_RESIZE:
    {
        if (!arg)
            return -EINVAL;

        struct fb_resize_args *resize = (struct fb_resize_args *)arg;

        
        return resize_frame_buffer_vga(
            fb_id,
            resize->new_width,
            resize->new_height,
            resize->new_bpp);
    }

    default:
        
        return 0;
    }
}
uint64_t sys_ioctl(uint64_t fd, uint64_t request, uint64_t arg)
{
    struct file_inner *file = (struct file_inner *)&((struct file_inner *)processes_info_ptr[get_current_pid()]->fd_addr)[fd];
    // if (!file)
    //     return -1;

    switch (file->type)
    {
    case FILE_NONE:
        return -1;

    case FILE_REGULAR:
        return sys_ioctl_file(file->global_id, request, arg);
        
        
        return 0;

    case FILE_CHAR_DEV:
        
        // vd: return driver_ioctl(file->res_id, request, arg);
        return 0;

    case FILE_DIR:
        
        return -1;

    case FILE_PIPE:
        return sys_ioctl_pipe(file->res_id, request, arg);

    case FILE_SOCKET:
        return sys_ioctl_socket(file->res_id, request, arg);

    case FILE_TTY:
        return sys_ioctl_tty(file->res_id, request, (void *)arg);

    case FILE_FRAME_BUFFER:
        return sys_ioctl_frame_buffer(file->res_id, request, (void *)arg);
    }

    return -1;
}
#endif // SYSCALL1_H
