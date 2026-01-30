#ifndef SYSCALL4_H
#define SYSCALL4_H

#define FUTEX_CMD_MASK 0x3f
#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_REQUEUE 3
uint64_t sys_futex(uint32_t *uaddr, uint32_t op, uint32_t val, struct timespec *timeout, uint32_t *uaddr2, uint32_t val3)
{
    switch (op & FUTEX_CMD_MASK)
    {
    case FUTEX_WAIT:
        return pause_thread_futex(get_current_thread_id(), uaddr, val, timeout);
    case FUTEX_WAKE:
        return resume_thread_futex(uaddr);
    case FUTEX_REQUEUE:
        return move_futex(uaddr, op, uaddr2, val3);
    };
}
uint64_t sys_mount(char *src, char *dst, char *filesystemtype, uint64_t flags, void *data)
{
    return mount(src, dst, flags);
}
uint64_t sys_get_main_tid(uint64_t pid)
{
    return processes_info_ptr[pid]->main_thread_id;
}
// uint64_t sys_create_process_winpe( uint16_t* path, uint16_t* cmd_line, uint16_t* envp, uint16_t* current_dir)
// {
//     // Create a new process in the Windows PE format
//     char path_str[256];
//     char cmd_str[256];
//     char envp_str[256];
//     char current_dir_str[256];
//     utf16_to_utf8((uint16_t *)path, ustrlen((UNICODE_STRING *)path), (uint8_t *)path_str, sizeof(path_str));
//     utf16_to_utf8((uint16_t *)cmd_line, ustrlen((UNICODE_STRING *)cmd_line), (uint8_t *)cmd_str, sizeof(cmd_str));
//     utf16_to_utf8((uint16_t *)envp, ustrlen((UNICODE_STRING *)envp), (uint8_t *)envp_str, sizeof(envp_str));
//     utf16_to_utf8((uint16_t *)current_dir, ustrlen((UNICODE_STRING *)current_dir), (uint8_t *)current_dir_str, sizeof(current_dir_str));
//     return run_exe_process( 0,0, path_str, cmd_str, envp_str, current_dir_str);
// }
void system_reboot(int x)
{
    cli();
    outb(0xCF9, 0x06);

    for (;;)
        halt();
    ;
}
#define SLP_EN (1 << 13)
#define SLP_TYP_S5 (5 << 10)
#define PM1_CNT SLP_TYP | SLP_EN
#define RB_RESTART 0
#define RB_POWEROFF 1
#define RB_HALT 2
void acpi_poweroff()
{
    cli();

    // Write SLP_TYP | SLP_EN
    outw(0x404, SLP_TYP_S5 | SLP_EN);

    
    for (;;)
        halt();
}
int64_t sys_reboot(int code)
{
    for (uint64_t i = 0;i<MAX_PROCESSES_COUNT;i++)
    {
        process_info_t* proc = processes_info_ptr[i];
        if (proc && proc->pid == 0)
        {
            continue;
        }
        mark_process_as_ended(i);
    }
    if (code == RB_RESTART)
    {
        system_reboot(0);
    }
    else if (code == RB_POWEROFF)
    {
        acpi_poweroff();
    }
    else if (code == RB_HALT)
    {
        cli();
        for (;;)
            halt();
    }
    else
        return -1;
}
#endif // SYSCALL4_H
