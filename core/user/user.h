#ifndef USER_H
#define USER_H

#define MAX_USER 256
struct user
{
    uint64_t uid;
    uint64_t gid;
};
//===========================================================
int64_t sys_getuid()
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
        return curr->uid;
    return -1;
}
int64_t sys_getgid()
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
        return curr->gid;
    return -1;
}
int64_t sys_setuid(uint64_t uid)
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
    {
        curr->uid = uid;
        return 0;
    }
    return -1;
}
int64_t sys_setgid(uint64_t gid)
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
    {
        curr->uid = gid;
        return 0;
    }
    return -1;
}
//====================================================
int64_t sys_geteuid()
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
        return curr->euid;
    return -1;
}
int64_t sys_getegid()
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
        return curr->egid;
    return -1;
}
int64_t sys_seteuid(uint64_t euid)
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
    {
        curr->euid = euid;
        return 0;
    }
    return -1;
}
int64_t sys_setegid(uint64_t egid)
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
    {
        curr->egid = egid;
        return 0;
    }
    return -1;
}
//===========================================================
int64_t sys_getreuid()
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
        return curr->reuid;
    return -1;
}
int64_t sys_getregid()
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
        return curr->regid;
    return -1;
}
int64_t sys_setreuid(uint64_t reuid)
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
    {
        curr->reuid = reuid;
        return 0;
    }
    return -1;
}
int64_t sys_setregid(uint64_t regid)
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
    {
        curr->regid = regid;
        return 0;
    }
    return -1;
}
//===========================================================
int64_t sys_getresuid()
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
        return curr->resuid;
    return -1;
}
int64_t sys_getresgid()
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
        return curr->resgid;
    return -1;
}
int64_t sys_setresuid(uint64_t resuid)
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
    {
        curr->resuid = resuid;
        return 0;
    }
    return -1;
}
int64_t sys_setresgid(uint64_t resgid)
{
    uint64_t pid = get_current_pid();
    process_info_t *curr = processes_info_ptr[pid];
    if (curr)
    {
        curr->resgid = resgid;
        return 0;
    }
    return -1;
}
//=======================================================
//=======================================================

extern struct user user_global[MAX_USER];
bool get_str_at_lv(char *path, uint64_t level, char *buf, uint64_t buf_size)
{
    uint64_t count_slashes = 0;
    uint64_t start_pos = 0;
    uint64_t i = 0;
    for (; i < strlen(path); i++)
    {
        if (path[i] == ':')
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
        if (path[i] == ':')
        {
            stop_pos = i;
            break;
        }
    }
    memcpy(buf, path + start_pos + 1, stop_pos - start_pos - 1);
    buf[stop_pos - start_pos - 1] = 0;
    return true;
}
int64_t get_uid_from_user_line(char *user_line)
{
    char out[32];
    get_str_at_lv(user_line, 2, out, 32);
    return str_to_int(out);
}
int64_t get_gid_from_user_line(char *user_line)
{
    char out[32];
    get_str_at_lv(user_line, 3, out, 32);
    return str_to_int(out);
}
void init_user()
{
    char user_file[2048];
    if (read_file("/etc/passwd", user_file, 0, 2048) <= 0)
    {
        return;
    }
    uint64_t file_size = strlen(user_file);
    char *line = user_file;
    while (*line)
    {
        if (*line == '\n')
        {
            *line = '\0';
        }
        line++;
    }
    char *user_liner = user_file;
    while (user_liner < user_file + file_size)
    {
        int64_t uid = get_uid_from_user_line(user_liner);
        int64_t gid = get_gid_from_user_line(user_liner);
        user_global[uid].uid = uid;
        user_global[uid].gid = gid;
        user_liner += strlen(user_liner) + 1;
    }
}
#define MODE_READ 4
#define MODE_WRITE 2
#define MODE_EXEC 1
int get_owner_file(char *path)
{
    return 0; // dummy;
}
int get_owner_group_file(char *path)
{
    return 0; // dummy;
}
int get_mode_file(char *path)
{
    return 0777; // dummy;
};
bool allow_read_path(char *path)
{
    // return true;
    uint64_t pid = get_current_pid();
    process_info_t *proc = processes_info_ptr[pid];
    uint64_t euid = sys_geteuid();
    uint64_t egid = sys_getegid();
    uint64_t file_owner_uid = get_owner_file(path);
    uint64_t file_owner_gid = get_owner_group_file(path);
    int file_mode = get_mode_file(path);
    if (euid == 0)
        return true;
    if (euid == file_owner_uid && file_mode & (MODE_READ >> 6) > 0)
        return true;
    if (egid == file_owner_gid && file_mode & (MODE_READ >> 3) > 0)
        return true;
    return file_mode & (MODE_READ) > 0;
}
bool allow_write_path(char *path)
{
    // return true;
    uint64_t pid = get_current_pid();
    process_info_t *proc = processes_info_ptr[pid];
    uint64_t euid = sys_geteuid();
    uint64_t egid = sys_getegid();
    uint64_t file_owner_uid = get_owner_file(path);
    uint64_t file_owner_gid = get_owner_group_file(path);
    int file_mode = get_mode_file(path);
    if (euid == 0)
        return true;
    if (euid == file_owner_uid && file_mode & (MODE_WRITE >> 6) > 0)
        return true;
    if (egid == file_owner_gid && file_mode & (MODE_WRITE >> 3) > 0)
        return true;
    return file_mode & (MODE_WRITE) > 0;
}
bool allow_exec_path(char *path)
{
    // return true;
    uint64_t pid = get_current_pid();
    process_info_t *proc = processes_info_ptr[pid];
    uint64_t euid = sys_geteuid();
    uint64_t egid = sys_getegid();
    uint64_t file_owner_uid = get_owner_file(path);
    uint64_t file_owner_gid = get_owner_group_file(path);
    int file_mode = get_mode_file(path);
    if (euid == 0)
        return true;
    if (euid == file_owner_uid && file_mode & (MODE_EXEC >> 6) > 0)
        return true;
    if (egid == file_owner_gid && file_mode & (MODE_EXEC >> 3) > 0)
        return true;
    return file_mode & (MODE_EXEC) > 0;
}
bool allow_root()
{
    return sys_getegid() == 0;
}
#endif
