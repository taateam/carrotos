#ifndef SPECIAL_FILES_H
#define SPECIAL_FILES_H

bool is_special_dirent(char *path)
{
    char *list_of_system_folders = "/sys/\0/proc/\0/dev/";
    uint64_t pos = (uint64_t)list_of_system_folders;
    while (1)
    {
        if (memcmp(path, (char *)pos, 5) == 0)
        {
            return true;
        }
        pos += strlen((char *)pos) + 1;
    }
    return false;
}
bool get_path_at_lv(char *path, uint64_t level, char *buf, uint64_t buf_size)
{
    uint64_t count_slashes = 0;
    uint64_t start_pos = 0;
    uint64_t i = 0;
    for (; i < strlen(path); i++)
    {
        if (path[i] == '/')
        {
            count_slashes++;
        }
        if (count_slashes >= level)
        {
            start_pos = i;
            break;
        }
    }
    if (i == strlen(path))
        return false;
    uint64_t stop_pos = strlen(path);
    for (i++; i < strlen(path); i++)
    {
        if (path[i] == '/')
        {
            stop_pos = i;
            break;
        }
    }
    memcpy(buf, path + start_pos + 1, stop_pos - start_pos - 1);
    buf[stop_pos - start_pos - 1] = 0;
    return true;
}
uint64_t count_path_level(char *path)
{
    uint64_t result = 0;
    for (uint64_t i = 0; i < strlen(path) - 1 /* ignore last '/' */; i++)
    {
        if (path[i] == '/')
        {
            result++;
        }
    }
    return result;
}
int64_t open_proc_file(char *path, uint64_t flags, uint64_t mode)
{
    uint64_t pid = 0;
    char pid_str[256];
    if (!get_path_at_lv(path, 2, pid_str, 256))
        return -EINVAL;
    pid = str_to_int(pid_str);
    process_info_t *target_process = processes_info_ptr[pid];
    char filename[256];
    if (!get_path_at_lv(path, 3, filename, 256))
        return -EINVAL;
    if (count_path_level(path) == 3)
    {
        char *valid_lv3_file = "cmdline\0environ\0status\0stat\0maps\0mem\0cwd\0exe";
        for (char *p = valid_lv3_file; *p != '\0'; p += strlen(p) + 1)
        {
            if (strequal(filename, p))
            {
                return 0;
            }
        }
    }
    if (count_path_level(path) == 4)
    {
        char *valid_lv3_folder = "fd\0task\0";
        for (char *p = valid_lv3_folder; *p != '\0'; p += strlen(p) + 1)
        {
            if (strequal(filename, p))
            {
                return 0;
            }
        }
    }
    return -EINVAL;
}
int64_t open_sys_file(char *path, int flags, int mode)
{
    return -EINVAL;
}
int64_t open_dev_file(char *path, int flags, int mode)
{
    return -EINVAL;
}
int64_t sys_open_special_dirent(char *path, uint64_t flags, uint64_t mode)
{
    if (memcmp(path, "/proc/", 6) == 0)
    {
        // Handle proc file opening
        int64_t rs = open_proc_file(path, flags, mode);
        if (rs < 0)
            return rs;
    }
    else if (memcmp(path, "/dev/", 5) == 0)
    {
        // Handle dev file opening
        int64_t rs = open_dev_file(path, flags, mode);
        if (rs < 0)
            return rs;
    }
    else if (memcmp(path, "/sys/", 5) == 0)
    {
        // Handle sys file opening
        int64_t rs = open_sys_file(path, flags, mode);
        if (rs < 0)
            return rs;
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
    file_inner_tmp->type = FILE_SPECIAL;
    file_global_tmp->offset = 0;
    file_global_tmp->open_flags = flags;
    file_global_tmp->type = FILE_SPECIAL;

    file_inner_tmp->global_id = fd_id_global;
    file_global_tmp->id = fd_id_global;

    return fd_id_inner;
}
//=================================================================================
void get_exe_name(uint64_t pid, char *exe_name)
{
    process_info_t *target_process = processes_info_ptr[pid];
    if (!target_process)
        return;
    // Extract the executable name from the command line
    char *cmd_line = target_process->cmd_line;
    uint64_t i = 0;
    while (cmd_line[i] != ' ' && cmd_line[i] != '\0' && i < MAX_CMD_LINE_LEN)
    {
        exe_name[i] = cmd_line[i];
        i++;
    }
    exe_name[i] = '\0';
}
void get_last_exe_name(uint64_t pid, char *exe_name)
{
    process_info_t *target_process = processes_info_ptr[pid];
    if (!target_process)
        return;
    char *cmd_line = target_process->cmd_line;
    // Extract the executable name from the command line
    uint64_t i = strlen(target_process->cmd_line);
    for (; i >= 0; i--)
    {
        if (cmd_line[i] == '/')
        {
            strcpy(exe_name, &cmd_line[i + 1]);
            break;
        }
    }
    exe_name[i] = '\0';
}
void process_state_to_str(process_state_t state, char *buf)
{
    switch (state)
    {
    case PROCESS_NOT_EXIST:
        strcpy(buf, "Not Exist");
        break;
    case PROCESS_RUNNING:
        strcpy(buf, "Running");
        break;
    case PROCESS_IDLE:
        strcpy(buf, "Idle");
        break;
    case PROCESS_STOPPED:
        strcpy(buf, "Stopped");
        break;
    case PROCESS_ENDED_BUT_NOT_CLEANED:
        strcpy(buf, "Ended But Not Cleaned");
        break;
    case PROCESS_HANDLING_SIGNAL:
        strcpy(buf, "Handling Signal");
        break;
    default:
        strcpy(buf, "Unknown");
        break;
    }
}
void read_status_proc(uint8_t *buf, uint64_t pid, uint64_t count)
{
    process_info_t *target_process = processes_info_ptr[pid];
    if (!target_process)
        return;

    // Read the status information into the buffer
    char status_info[1024];
    strconcat(status_info, status_info, "Name: ");
    get_last_exe_name(pid, status_info + strlen(status_info));
    strconcat(status_info, status_info, "\n");

    strconcat(status_info, status_info, "State: ");
    char state_str[20];
    process_state_to_str(target_process->state, state_str);
    strconcat(status_info, status_info, state_str);
    strconcat(status_info, status_info, "\n");

    strconcat(status_info, status_info, "Pid: ");
    char pid_str[20];
    int_to_str(pid, pid_str);
    strconcat(status_info, status_info, pid_str);
    strconcat(status_info, status_info, "\n");

    strconcat(status_info, status_info, "Ppid: ");
    char ppid_str[20];
    int_to_str(target_process->parent_pid, ppid_str);
    strconcat(status_info, status_info, ppid_str);
    strconcat(status_info, status_info, "\n");

    strconcat(status_info, status_info, "Uid: ");
    char uid_str[20];
    int_to_str(target_process->uid, uid_str);
    strconcat(status_info, status_info, uid_str);
    strconcat(status_info, status_info, "\n");

    strconcat(status_info, status_info, "Gid: ");
    char gid_str[20];
    int_to_str(target_process->gid, gid_str);
    strconcat(status_info, status_info, gid_str);
    strconcat(status_info, status_info, "\n");

    strconcat(status_info, status_info, "Threads: ");
    char threads_str[20];
    int_to_str(target_process->threads_count, threads_str);
    strconcat(status_info, status_info, threads_str);
    strconcat(status_info, status_info, "\n");

    strconcat(status_info, status_info, "VmSize: ");
    char vmsize_str[20];
    int_to_str(0, vmsize_str);
    strconcat(status_info, status_info, vmsize_str);
    strconcat(status_info, status_info, " kB\n");

    strconcat(status_info, status_info, "VmRSS: ");
    char vmrss_str[20];
    int_to_str(0, vmrss_str);
    strconcat(status_info, status_info, vmrss_str);
    strconcat(status_info, status_info, " kB\n");

    // Copy the status information to the output buffer
    memcpy(buf, status_info, count < strlen(status_info) ? count : strlen(status_info));
}
void read_status_stat(uint8_t *buf, uint64_t pid, uint64_t count)
{
    process_info_t *target_process = processes_info_ptr[pid];
    if (!target_process)
        return;

    // Read the status information into the buffer
    char status_info[1024];
    get_last_exe_name(pid, status_info + strlen(status_info));
    strconcat(status_info, status_info, " ");

    char state_str[20];
    process_state_to_str(target_process->state, state_str);
    strconcat(status_info, status_info, state_str);
    strconcat(status_info, status_info, " ");

    char pid_str[20];
    int_to_str(pid, pid_str);
    strconcat(status_info, status_info, pid_str);
    strconcat(status_info, status_info, " ");

    char ppid_str[20];
    int_to_str(target_process->parent_pid, ppid_str);
    strconcat(status_info, status_info, ppid_str);
    strconcat(status_info, status_info, " ");

    char uid_str[20];
    int_to_str(target_process->uid, uid_str);
    strconcat(status_info, status_info, uid_str);
    strconcat(status_info, status_info, " ");

    char gid_str[20];
    int_to_str(target_process->gid, gid_str);
    strconcat(status_info, status_info, gid_str);
    strconcat(status_info, status_info, " ");

    char threads_str[20];
    int_to_str(target_process->threads_count, threads_str);
    strconcat(status_info, status_info, threads_str);
    strconcat(status_info, status_info, " ");

    char vmsize_str[20];
    int_to_str(0, vmsize_str);
    strconcat(status_info, status_info, vmsize_str);
    strconcat(status_info, status_info, " ");

    char vms_str[20];
    int_to_str(0, vms_str);
    strconcat(status_info, status_info, vms_str);
}
void read_maps_proc(uint8_t *buf, uint64_t pid, uint64_t count)
{
    return; // TODO:
    // process_info_t *target_process = processes_info_ptr[pid];
    // if (!target_process)
    //     return;

    // // Read the memory map information into the buffer
    // char map_info[1024];
    // get_last_exe_name(pid, map_info + strlen(map_info));
    // strconcat(map_info, map_info, " ");

    // // Iterate over the memory regions and concatenate their information
    // for (int i = 0; i < MAX_MEMORY_REGIONS; i++) {
    //     memory_region_t *region = &target_process->memory_regions[i];
    //     if (!region->valid)
    //         continue;

    //     char region_info[256];
    //     snprintf(region_info, sizeof(region_info), "[%lx-%lx] %s %s\n",
    //              region->start, region->end,
    //              region->permissions, region->path);
    //     strconcat(map_info, map_info, region_info);
    // }

    // // Copy the map information to the output buffer
    // memcpy(buf, map_info, count);
}
void read_mem_proc(uint8_t *buf, uint64_t pid, uint64_t count)
{
    return; // TODO:
}
void read_proc_cwd(uint8_t *buf, uint64_t pid, uint64_t count)
{
    process_info_t *target_process = processes_info_ptr[pid];
    if (!target_process)
        return;
    memcpy(buf, target_process->cwd, strlen(target_process->cwd) < count ? strlen(target_process->cwd) : count);
}
void read_proc_exe(uint8_t *buf, uint64_t pid, uint64_t count)
{
    process_info_t *target_process = processes_info_ptr[pid];
    if (!target_process)
        return;
    char exe[256];
    get_exe_name(pid, exe);
    memcpy(buf, exe, strlen(exe) < count ? strlen(exe) : count);
}
int64_t read_fd_file(char *fd_str, uint8_t *buf, uint64_t pid, uint64_t count)
{
    // Implement the logic to read from a file descriptor
    int fd = str_to_int(fd_str);
    process_info_t *target_process = processes_info_ptr[pid];
    if (!target_process)
        return -EINVAL;

    // Read from the file descriptor
    struct file_inner *file = &((struct file_inner *)(target_process->fd_addr))[fd];
    struct file *global_file = fd_global[file->global_id];
    memcpy(buf, global_file->path, strlen(global_file->path) < count ? strlen(global_file->path) : count);
}
int64_t read_proc_file(char *path, uint8_t *buf, uint64_t start, uint64_t count)
{
    // Implement the logic to read from a proc file
    uint64_t pid = 0;
    char pid_str[20];
    if (!get_path_at_lv(path, 2, pid_str, 20))
        return -EINVAL;
    pid = str_to_int(pid_str);
    process_info_t *target_process = processes_info_ptr[pid];
    char filename[DIRENT_NAME_MAX_LENGTH];
    if (!get_path_at_lv(path, 3, filename, DIRENT_NAME_MAX_LENGTH))
        return -EINVAL;
    if (count_path_level(path) == 3)
    {
        if (strequal(filename, "cmdline"))
        {
            memcpy(buf, target_process->cmd_line, count);
            return count;
        }
        else if (strequal(filename, "environ"))
        {
            memcpy(buf, target_process->env, count);
            return count;
        }
        else if (strequal(filename, "status"))
        {
            read_status_proc(buf, pid, count);
            return count;
        }
        else if (strequal(filename, "stat"))
        {
            read_status_stat(buf, pid, count);
            return count;
        }
        else if (strequal(filename, "maps"))
        {
            read_maps_proc(buf, pid, count);
            return count;
        }
        else if (strequal(filename, "mem"))
        {
            read_mem_proc(buf, pid, count);
            return count;
        }
        else if (strequal(filename, "cwd"))
        {
            read_proc_cwd(buf, pid, count);
            return count;
        }
        else if (strequal(filename, "exe"))
        {
            read_proc_exe(buf, pid, count);
            return count;
        }
    }
    if (count_path_level(path) == 4)
    {
        char *valid_lv3_folder = "fd\0task\0";
        if (strequal(filename, "fd"))
        {
            // Implement reading from /proc/[pid]/fd
            char fd_str[20];
            get_child_path_at_level(4, path, fd_str);
            return read_fd_file(fd_str, buf, pid, count);
        }
        else if (strequal(filename, "task"))
        {
            // Implement reading from /proc/[pid]/task
            return -EINVAL; // Placeholder
        }
    }
    return -EINVAL;
}
int64_t read_sys_file(char *path, uint8_t *buf, uint64_t start, uint64_t count)
{
    return 0;
}
int64_t read_dev_file(char *path, uint8_t *buf, uint64_t start, uint64_t count)
{
    for (uint64_t i = 0; i < MAX_DEVICE; i++)
    {
        if (device_global[i] && strequal(device_global[i]->path, path))
        {
            if (device_global[i]->driver && device_global[i]->driver->read)
            {
                return device_global[i]->driver->read(device_global[i], start / 4096, buf);
            }
            else
            {
                return -EINVAL;
            }
        }
    }
}
int64_t sys_read_special_file(uint64_t fd, uint8_t *buf, uint64_t count)
{
    struct file_inner *file = (struct file_inner *)&((struct file_inner *)processes_info_ptr[get_current_pid()]->fd_addr)[fd];
    uint64_t start_pos = file->offset;
    char *path = fd_global[file->global_id]->path;
    if (memcmp(path, "/proc/", 6) == 0)
    {
        // Handle proc file opening
        int64_t rs = read_proc_file(path, buf, start_pos, count);
        if (rs >= 0)
            return rs;
    }
    else if (memcmp(path, "/dev/", 5) == 0)
    {
        // Handle dev file opening
        int64_t rs = read_dev_file(path, buf, start_pos, count);
        if (rs >= 0)
            return rs;
    }
    else if (memcmp(path, "/sys/", 5) == 0)
    {
        // Handle sys file opening
        int64_t rs = read_sys_file(path, buf, start_pos, count);
        if (rs >= 0)
            return rs;
    }

    return -EINVAL;
};
//============================================================
void create_dirent_in_list(struct dirent_in_list *output, char *fake_name, uint64_t fake_offset, bool is_dir)
{
    if (!output || !fake_name)
        return;

    output->d_ino = 0; // Fake inode number
    output->d_off = fake_offset;
    output->d_reclen = sizeof(struct dirent_in_list) + strlen(fake_name) + 1;
    output->d_type = is_dir ? DT_DIR : DT_REG;

    // Copy the fake name into the dirent structure
    strcpy(output->d_name, fake_name);
}
int64_t sys_list_dir_special_file(uint64_t fd, uint8_t *buf, uint64_t count)
{
    struct file_inner *file = (struct file_inner *)&((struct file_inner *)processes_info_ptr[get_current_pid()]->fd_addr)[fd];
    char *path = fd_global[file->global_id]->path;
    if (strequal(path, "/proc") || strequal(path, "/proc/"))
    {
        uint64_t byte_written = 0;
        uint64_t i = 0;
        while (1)
        {
            if (i >= MAX_PROCESSES_COUNT || processes_info_ptr[i]->state == PROCESS_NOT_EXIST)
                break;
            if (byte_written + sizeof(struct dirent_in_list) + 20 > count)
                break;
            char pid_str[20];
            int_to_str(i, pid_str);
            create_dirent_in_list((struct dirent_in_list *)(buf + byte_written), pid_str, i + 1, true);
            byte_written += sizeof(struct dirent_in_list) + strlen(pid_str) + 1;
            i++;
        }
        return byte_written;
        // List /proc directory
    }
    else if (strequal(path, "/dev"))
    {
        // List /dev directory
        return 0;
    }
    else if (strequal(path, "/sys"))
    {
        // List /sys directory
        return 0;
    }
    else if (memcmp(path, "/proc/", 6) == 0)
    {
        // List /proc/[pid] directory
        char pid_str[20];
        get_path_at_lv(path, 2, pid_str, 20);
        uint64_t pid = str_to_int(pid_str);
        if (count_path_level(path) == 2)
        {
            uint64_t byte_written = 0;
            char *lv3_files = "cmdline\0environ\0status\0stat\0maps\0mem\0cwd\0exe\0";
            for (char *p = lv3_files; *p != '\0'; p += strlen(p) + 1)
            {
                if (byte_written + sizeof(struct dirent_in_list) + strlen(p) + 1 > count)
                    break;
                bool is_dir = strequal(p, "fd") || strequal(p, "task");
                create_dirent_in_list((struct dirent_in_list *)(buf + byte_written), p, byte_written + 1, is_dir);
                byte_written += sizeof(struct dirent_in_list) + strlen(p) + 1;
            }
            return byte_written;
        }
        char lv3_name[DIRENT_NAME_MAX_LENGTH];
        get_path_at_lv(path, 3, lv3_name, DIRENT_NAME_MAX_LENGTH);
        if (count_path_level(path) == 3 && (strequal(lv3_name, "fd")))
        {
            struct file_inner *file = (struct file_inner *)&((struct file_inner *)processes_info_ptr[pid]->fd_addr)[0];
            uint64_t byte_written = 0;
            for (uint64_t i = 0; i < MAX_FD_INNER; i++)
            {
                if (file[i].type == FILE_NONE)
                    continue;
                char fd_str[20];
                int_to_str(i, fd_str);
                create_dirent_in_list((struct dirent_in_list *)(buf + byte_written), fd_str, byte_written + 1, true);
                byte_written += sizeof(struct dirent_in_list) + strlen(fd_str) + 1;
            }
            return byte_written;
        }
    }
    else if (memcmp(path, "/dev/", 5) == 0)
    {
        // List /dev/[device] directory
        return 0;
    }
    else if (memcmp(path, "/sys/", 5) == 0)
    {
        // List /sys/[device] directory
        return 0;
    }
    else if (memcmp(path, "", 0) == 0)
    {
        return -EINVAL;
    }
}
//=============================================================
uint64_t sys_write_special_file(uint64_t fd, uint8_t *buf, uint64_t count)
{
    return -EINVAL;
}
#endif // SPECIAL_FILE_H
