#ifndef SYSCALL2_H
#define SYSCALL2_H

#include <stdint.h>
uint64_t pageline_to_low_addr(uint64_t page_line)
{
    return page_line & 0x000FFFFFFFFFF000ULL;
}
// TODO: duplicated
uint64_t pageline_to_high_addr(uint64_t page_line)
{
    return (page_line & 0x000FFFFFFFFFF000ULL) + HM;
}
uint64_t pageline_get_flags(uint64_t pageline)
{
    return pageline & 0xFFF;
}
uint64_t make_pageline(uint64_t addr, uint64_t flags)
{
    return (addr & ~0xFFFULL) | (flags & 0xFFFULL);
}
void clone_cr3(uint64_t *l4_original, uint64_t *l4_new)
{
    // low memory
    uint64_t high_mem_starting_l4i = HM >> 39;
    uint64_t l4i, l3i, l2i, l1i = 0;
    for (l4i = 0; l4i < high_mem_starting_l4i; l4i++)
    {
        if (l4_original[l4i] == 0)
            continue;
        uint64_t new_l4_page_addr = get_a_free_block_addr();
        uint64_t l4_flags = pageline_get_flags(l4_original[l4i]);
        uint64_t new_l4_pageline = make_pageline(new_l4_page_addr - HM, l4_flags);
        l4_new[l4i] = new_l4_pageline;
        uint64_t *l3_original = (uint64_t *)pageline_to_high_addr(l4_original[l4i]);
        uint64_t *l3_new = (uint64_t *)new_l4_page_addr;
        for (l3i = 0; l3i < 512; l3i++)
        {
            if (l3_original[l3i] == 0)
                continue;
            uint64_t new_l3_page_addr = get_a_free_block_addr();
            uint64_t l3_flags = pageline_get_flags(l3_original[l3i]);
            uint64_t new_l3_pageline = make_pageline(new_l3_page_addr - HM, l3_flags);
            l3_new[l3i] = new_l3_pageline;
            uint64_t *l2_original = (uint64_t *)pageline_to_high_addr(l3_original[l3i]);
            uint64_t *l2_new = (uint64_t *)new_l3_page_addr;
            for (l2i = 0; l2i < 512; l2i++)
            {
                if (l2_original[l2i] == 0)
                    continue;
                uint64_t new_l2_page_addr = get_a_free_block_addr();
                uint64_t l2_flags = pageline_get_flags(l2_original[l2i]);
                uint64_t new_l2_pageline = make_pageline(new_l2_page_addr - HM, l2_flags);
                l2_new[l2i] = new_l2_pageline;
                uint64_t *l1_original = (uint64_t *)pageline_to_high_addr(l2_original[l2i]);
                uint64_t *l1_new = (uint64_t *)new_l2_page_addr;
                memcpy((uint8_t *)l1_new, (uint8_t *)l1_original, 4096);
            };
        };
    }
    // high memory
    for (l4i = high_mem_starting_l4i; l4i < 512; l4i++)
    {
        l4_new[l4i] = l4_original[l4i];
    };
}
extern void fork_child_ret();
uint64_t sys_fork()
{
    uint64_t parent_pid = get_current_pid();
    process_info_t *parent_proc = processes_info_ptr[parent_pid];
    thread_info_t *parent_thread = threads_info_ptr[get_current_thread_id()];
    // create new process
    uint64_t child_pid = get_a_free_slot((uint64_t)&processes_info_ptr, 1, MAX_PROCESSES_COUNT);
    process_info_t *child_proc = (process_info_t *)get_a_free_block_addr();
    processes_info_ptr[child_pid] = child_proc;
    processes_cr3_ptr[child_pid] = processes_cr3_ptr[parent_pid];
    // copy and create child process info
    memcpy((uint8_t *)child_proc, (uint8_t *)parent_proc, sizeof(process_info_t));
    child_proc->pid = child_pid;
    child_proc->parent_pid = parent_proc->pid;
    child_proc->threads_count = 1;
    child_proc->state = PROCESS_RUNNING;
    // handle fd
    struct file_inner *fd_parent = (struct file_inner *)parent_proc->fd_addr;
    struct file_inner *fd_child = (struct file_inner *)(get_continuous_free_blocks(20) * 4096 + HM);
    erase_mem8((uint64_t)fd_child, 20 * 4096);
    for (uint64_t i = 0; i < MAX_FD_INNER; i++)
    {
        if (i <= 2 || fd_parent[i].type != FILE_PIPE)
        {
            memcpy((uint8_t *)&fd_child[i], (uint8_t *)&fd_parent[i], sizeof(struct file_inner));
        };
    }
    // thread
    uint64_t child_tid = get_a_free_slot((uint64_t)&threads_info_ptr, 1, MAX_THREADS_COUNT);
    thread_info_t *child_thread = (thread_info_t *)(get_continuous_free_blocks(5) * 4096 + HM); // including kernel stack in these blocks;
    threads_info_ptr[child_tid] = child_thread;
    // copy and create child thread info
    // declare all vars to unify stack structure
    uint64_t parent_kernel_stack_top, child_kernel_stack_top, kernel_stack_addr;
    uint64_t parent_rsp_current, parent_rbp_current, child_thread_rsp, new_cr3_addr, tmp;
    int64_t stack_distant = 0;
    // clone thread info and kernel stack
    memcpy((uint8_t *)child_thread, (uint8_t *)parent_thread, 5 * 4096); // including kernel stack in these blocks;
    child_thread->state = THREAD_READY;
    child_thread->tid = child_tid;
    child_thread->pid = child_pid;
    parent_kernel_stack_top = parent_thread->kernel_stack_addr;
    child_kernel_stack_top = ((uint64_t)child_thread) + 5 * 4096;
    child_thread->kernel_stack_addr = child_kernel_stack_top;
    // context child;
    child_thread->context.ss = 0x10;
    child_thread->context.cs = 0x08;
    child_thread->context.rip = (uint64_t)fork_child_ret;
    stack_distant = child_kernel_stack_top - parent_kernel_stack_top;
    child_thread->context.rdx = stack_distant;
    __asm__ volatile("mov %%rsp, %0" : "=r"(parent_rsp_current));
    child_thread_rsp = child_kernel_stack_top - (parent_kernel_stack_top - parent_rsp_current);
    child_thread->context.rsp = child_thread_rsp;
    __asm__ volatile("mov %%rbp, %0" : "=r"(parent_rbp_current));
    child_thread->context.rbp = child_kernel_stack_top - (parent_kernel_stack_top - parent_rbp_current);
    child_proc->main_thread_id = child_tid;
    __asm__ volatile("movq %%gs:24, %0" : "=r"(child_thread->context.gs24));
    //=====================
    new_cr3_addr = get_a_free_block_addr();
    clone_cr3((uint64_t *)processes_cr3_ptr[parent_pid], (uint64_t *)new_cr3_addr);
    processes_cr3_ptr[child_pid] = new_cr3_addr;
    for (int i = 0; i < 4; i++)
    {
        decrease_paging_of_process_with_virt_addr(child_pid, child_thread->stack_addr - (i + 1) * 4096);
        tmp = increase_paging_of_process_with_virt_addr(child_pid, child_thread->stack_addr - (i + 1) * 4096) + HM;
        memcpy((uint8_t *)tmp, (uint8_t *)parent_thread->stack_addr - (i + 1) * 4096, 4096);
    }
    //========================
    return child_pid;
    // fork_child_ret:
    // return 0;
}
uint64_t sys_execve(char *path, char *argv[], char *envp[])
{
    uint64_t curr_pid = get_current_pid();
    uint64_t curr_tid_bk = get_current_thread_id();
    thread_info_t *curr_thread_bk = threads_info_ptr[curr_tid_bk];
    uint64_t curr_cr3_bk = processes_cr3_ptr[curr_pid];
    // copy path, argv, env,...
    uint64_t path_len = strlen(path);
    uint64_t argv_count = count_str_arr(argv);
    uint64_t total_argv_len = total_memsize_str_arr(argv);
    uint64_t envp_count = count_str_arr(envp);
    uint64_t total_envp_len = total_memsize_str_arr(envp);
    char _path[path_len + 1];
    uint8_t _argv_strs[total_argv_len];
    uint8_t _envp_strs[total_envp_len];
    char *_argv[argv_count];
    char *_envp[envp_count];
    strcpy(_path, path);
    copy_str_arr(argv, _argv_strs, _argv);
    copy_str_arr(envp, _envp_strs, _envp);
    // free user stack after fork
    uint64_t old_user_stack_addr = curr_thread_bk->stack_addr;
    for (int i = 1; i <= 4; i++)
    {
        decrease_paging_of_process_with_virt_addr(curr_pid, old_user_stack_addr - i * 4096);
    };
    run_elf_process(_path, _argv, _envp, true);
    free_page_hm(curr_cr3_bk, 4);
    uint64_t new_tid = processes_info_ptr[curr_pid]->main_thread_id;
    thread_info_t *new_thread = threads_info_ptr[new_tid];
    new_thread->context.gs24 = new_thread->stack_addr;
    end_thread(curr_tid_bk, 0);
    scheduler_yeild();
    return 0;
}
uint64_t sys_getpid()
{
    return get_current_pid();
}
uint64_t sys_getppid()
{
    uint64_t pid = get_current_pid();
    process_info_t *current_process = processes_info_ptr[pid];
    return current_process->parent_pid;
}
#define CLONE_THREAD 0x00010000
uint64_t sys_clone(unsigned long flags, void *child_stack_top, int *ptid, int *ctid, unsigned long newtls)
{
    if (!(flags & CLONE_THREAD))
        return -1;
    uint64_t pid = get_current_pid();
    process_info_t *proc = processes_info_ptr[pid];
    if (proc->state == PROCESS_ENDED_BUT_NOT_CLEANED)
    {
        return -1;
    }
    int ptid_val = ptid ? *ptid : 0;

    // uint64_t pid = get_current_pid();
    // process_info_t *proc = processes_info_ptr[pid];
    thread_info_t *parent_thread = threads_info_ptr[get_current_thread_id()];
    // thread
    uint64_t child_tid = get_a_free_slot((uint64_t)&threads_info_ptr, 1, MAX_THREADS_COUNT);
    thread_info_t *child_thread = (thread_info_t *)(get_continuous_free_blocks(5) * 4096 + HM); // including kernel stack in these blocks;
    threads_info_ptr[child_tid] = child_thread;
    // clone user stack
    uint64_t parent_thread_stack = parent_thread->stack_addr;
    // declare all vars to unify stack structure
    uint64_t parent_kernel_stack_top, child_kernel_stack_top, kernel_stack_addr;
    uint64_t parent_rsp_current, parent_rbp_current, child_thread_rsp, new_cr3_addr, tmp;
    int64_t stack_distant = 0;
    // clone thread info and kernel stack
    memcpy((uint8_t *)child_thread, (uint8_t *)parent_thread, 5 * 4096); // including kernel stack in these blocks;
    child_thread->state = THREAD_READY;
    child_thread->tid = child_tid;
    child_thread->pid = pid;
    parent_kernel_stack_top = parent_thread->kernel_stack_addr;
    child_kernel_stack_top = ((uint64_t)child_thread) + 5 * 4096;
    child_thread->kernel_stack_addr = child_kernel_stack_top;
    // context child;
    child_thread->context.ss = 0x10;
    child_thread->context.cs = 0x08;
    child_thread->context.rip = (uint64_t)fork_child_ret;
    stack_distant = child_kernel_stack_top - parent_kernel_stack_top;
    child_thread->context.rdx = stack_distant;
    __asm__ volatile("mov %%rsp, %0" : "=r"(parent_rsp_current));
    child_thread_rsp = child_kernel_stack_top - (parent_kernel_stack_top - parent_rsp_current);
    child_thread->context.rsp = child_thread_rsp;
    __asm__ volatile("mov %%rbp, %0" : "=r"(parent_rbp_current));
    child_thread->context.rbp = child_kernel_stack_top - (parent_kernel_stack_top - parent_rbp_current);
    child_thread->context.gs24 = (uint64_t)child_stack_top;
    //=====================
    //========================
    return child_tid;
}
void sys_exit(int status)
{
    mark_current_thread_as_ended(status);
}
void sys_exit_group(int status)
{
    mark_process_as_ended(status);
}
//===============================================================================================
// Protection flags: PROT_*
#define PROT_NONE 0x0  
#define PROT_READ 0x1  
#define PROT_WRITE 0x2 
#define PROT_EXEC 0x4  

#define MAP_SHARED 0x01          
#define MAP_PRIVATE 0x02         
#define MAP_SHARED_VALIDATE 0x03 // (Linux 4.15+) validate flags

// File vs anonymous
#define MAP_ANONYMOUS 0x20     
#define MAP_ANON MAP_ANONYMOUS 

// Fixed address behavior
#define MAP_FIXED 0x10               
#define MAP_FIXED_NOREPLACE 0x100000 

// Stack / grow-down
#define MAP_GROWSDOWN 0x0100 
#define MAP_STACK 0x20000    

// Pre-fault, locking
#define MAP_LOCKED 0x2000    // Lock page trong RAM
#define MAP_NORESERVE 0x4000 
#define MAP_POPULATE 0x8000  
#define MAP_NONBLOCK 0x10000 

// Huge pages / DAX
#define MAP_HUGETLB 0x40000 
#define MAP_SYNC 0x80000    // DAX sync

// Legacy / compat (optional)
#define MAP_FILE 0x00
#define MAP_DENYWRITE 0x0800
#define MAP_EXECUTABLE 0x1000
#define MAP_UNINITIALIZED 0x4000000

uint64_t sys_mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t flags, uint64_t fd, uint64_t offset)
{
    bool is_shared;
    if (fd >= 0 && (flags & MAP_SHARED))
    {
        is_shared = true; // map existing shared_mem
    }
    else
    {
        is_shared = false; // allocate new pages
    }
    uint64_t pid = get_current_pid();
    if (addr == 0)
    {
        // return -1;
        addr = find_free_virtual_region(pid, length); 
    }
    if (is_paged(pid, addr))
        return -EINVAL;
    if (is_shared)
    {
        create_sm(addr, length, flags);
        return addr;
    }
    else
    {
        
        uint64_t blocks_count = div_ceil(length, 4096);
        for (uint64_t i = 0; i < blocks_count; i++)
        {
            increase_paging_of_process_with_virt_addr(pid, addr + i * 4096);
        }
        if (!(flags & MAP_ANONYMOUS))
        {
            process_info_t **tmp = processes_info_ptr;
            struct file_inner *tmp1 = (struct file_inner *)tmp[pid]->fd_addr;
            read_file(fd_global[tmp1[fd].global_id]->path, (uint8_t *)addr, offset, length);
        }
    }
    return addr;
}
uint64_t sys_munmap(uint64_t addr, uint64_t length)
{
    uint64_t pid = get_current_pid();
    uint64_t blocks_count = div_ceil(length, 4096);
    for (uint64_t i = 0; i < blocks_count; i++)
    {
        decrease_paging_of_process_with_virt_addr(pid, addr + i * 4096);
    }
    return 0;
}
uint64_t sys_brk(uint64_t new_heap_end_addr)
{
    uint64_t tid = get_current_thread_id();
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    process_info_t *this_process = tmp[pid];
    // new_heap_end_addr += this_process->heap_start_addr;
    uint64_t current_heap_end_addr = this_process->heap_end_addr;
    this_process->heap_end_addr = new_heap_end_addr;
    if (new_heap_end_addr > current_heap_end_addr)
    {
        for (uint64_t i = div_ceil(current_heap_end_addr, 4096); i < div_ceil(new_heap_end_addr, 4096); i++)
        {
            increase_paging_of_process_with_virt_addr(pid, i * 4096);
        }
    }
    else
    {
        for (uint64_t i = div_ceil(current_heap_end_addr, 4096); i > div_ceil(new_heap_end_addr, 4096); i--)
        {
            decrease_paging_of_process_with_virt_addr(pid, (i - 1) * 4096);
        }
    }
    return new_heap_end_addr;
}
//================================================================================
#define SIGKILL 9
#define SIGSTOP 19

uint64_t sys_kill(uint64_t pid, uint8_t sig)
{
    if (sig > 63)
        return -EINVAL;

    process_info_t **tmp = processes_info_ptr;
    process_info_t *target_process = tmp[pid];

    thread_info_t **tmp1 = threads_info_ptr;

    if (!target_process || target_process->state == PROCESS_NOT_EXIST)
        return -ESRCH;

    
    send_signal_to_process(pid, sig, NULL);
    if (sig == SIGKILL)
    {
        mark_process_as_ended(pid);
        return 0;
    }
    if (sig == SIGSTOP)
    {
        for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
        {
            if (tmp1[i]->pid == pid && tmp1[i]->state == THREAD_READY)
                tmp1[i]->state = THREAD_BLOCKED_OTHER;
        };
        return 0;
    }
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (tmp1[i] && tmp1[i]->pid == pid && tmp1[i]->state == THREAD_READY)
            tmp1[i]->state = THREAD_BLOCKED_OTHER;
    };
    if (tmp[pid] && tmp[pid]->state == PROCESS_HANDLING_SIGNAL)
    {
        return -EAGAIN;
    }
    handle_signal_process(pid);
    return 0;
}

uint64_t sys_tgkill(uint64_t pid, uint64_t tid, int sig)
{
    if (sig < 1 || sig > 63)
        return -EINVAL;

    process_info_t **tmp = processes_info_ptr;
    process_info_t *proc = tmp[pid];
    if (proc->state == PROCESS_NOT_EXIST)
        return -ESRCH;

    thread_info_t **tmp1 = threads_info_ptr;
    thread_info_t *target_thread = tmp1[tid];
    if (target_thread->state == THREAD_NOT_EXIST)
        return -ESRCH;

    target_thread->signal = sig;
    handle_signal_thread(tid);
    return 0;
}
typedef void (*sighandler_t)(int sig);


struct sigaction
{
    sighandler_t sa_handler; 
    uint64_t sa_flags;       
    uint64_t sa_mask;        
};
long sys_rt_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize)
{
    if (signum > MAX_SIGNAL)
        return -EINVAL;
    if (signum == SIGKILL || signum == SIGSTOP)
        return -EINVAL; /* Linux returns EINVAL for these */
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    if (oldact != NULL)
    {
        struct sigaction tmp1;
        memcpy((uint8_t *)&tmp1.sa_handler, (uint8_t *)tmp[pid]->sig_handlers[signum], sizeof(sighandler_t));
        tmp1.sa_flags = 0;
        tmp1.sa_mask = 0;
        memcpy((uint8_t *)oldact, (uint8_t *)&tmp1, sizeof(struct sigaction));
    }
    if (act != NULL)
    {
        tmp[pid]->sig_handlers[signum] = (uint64_t)act->sa_handler;
    }
    return 0;
}
long sys_rt_sigreturn()
{
    process_info_t **tmp = processes_info_ptr;
    thread_info_t **tmp1 = threads_info_ptr;
    uint64_t pid = get_current_pid();
    uint64_t tid = get_current_thread_id();
    mark_current_thread_as_ended(tid);
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (tmp1[i] && tmp1[i]->pid == pid && tmp1[i]->state == THREAD_WAITING_SIGNAL)
            tmp1[i]->state = THREAD_READY;
    }
    tmp[pid]->state = PROCESS_RUNNING;
    tmp1[tid]->signal = 0;
    return 0;
}
#define SIG_BLOCK 0   
#define SIG_UNBLOCK 1 
#define SIG_SETMASK 2 
uint64_t sigprocmask(int how, uint64_t *set, uint64_t *oldset)
{
    thread_info_t *curr = threads_info_ptr[get_current_thread_id()];
    if (!set)
        return -EINVAL;
    if (!curr)
        return -ESRCH;

    if (oldset)
        *oldset = curr->signal_mask;

    switch (how)
    {
    case SIG_BLOCK:
        curr->signal_mask |= *set;
        break;
    case SIG_UNBLOCK:
        curr->signal_mask &= ~(*set);
        break;
    case SIG_SETMASK:
        curr->signal_mask = *set;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}
uint64_t rt_sigqueueinfo(int32_t pid, int32_t sig, struct signal *info)
{
    return send_signal_to_process(pid, sig, info);
}
uint64_t rt_tgsigqueueinfo(int32_t pid, int32_t tid, int sig, struct signal *info)
{
    return send_signal_to_thread(pid, tid, sig, info);
}
uint64_t sys_setpgid(uint64_t pid, uint64_t pgid)
{
    if (pid == 0)
        pid = get_current_pid();

    process_info_t *cur = processes_info_ptr[get_current_pid()];
    process_info_t *target = processes_info_ptr[pid];

    
    // if (target->has_execed)
    //     return -EPERM;

    
    if (pgid == 0)
        pgid = pid;

    
    if (target->sid != cur->sid)
        return -EPERM;

    
    if (target->parent_pid != cur->pid && target->pid != cur->pid)
        return -ESRCH;

    target->pgid = pgid;
    return 0; 
}
uint64_t sys_sleep(struct timespec *req, struct timespec *rem)
{
    user_thread_current_sleep(req);
}
uint64_t sys_setsid()
{
    process_info_t *cur = processes_info_ptr[get_current_pid()];

    
    if (cur->pid == cur->pgid)
        return -EPERM;

    
    cur->sid = cur->pid;

    
    cur->pgid = cur->pid;

    return cur->sid;
}
uint64_t sys_waitpid(uint64_t pid, uint64_t status_ptr, uint64_t options)
{
    process_info_t *parent = processes_info_ptr[get_current_pid()];
    thread_info_t *parent_thread = threads_info_ptr[get_current_thread_id()];
    parent_thread->state = THREAD_BLOCKED_PID;
    if (pid != -1)
    {
        parent_thread->sleep_flag_addr = (uint64_t)&processes_info_ptr[pid]->lock;
    }
    else
    {
        parent_thread->sleep_flag_addr = -1;
    }
    scheduler_yeild(); 
    return -ECHILD;
}
uint64_t sys_getpgid(uint64_t pid)
{
    if (pid == 0)
        pid = get_current_pid();

    process_info_t *p = processes_info_ptr[pid];
    if (!p)
        return -ESRCH;

    return p->pgid;
}
uint64_t sys_getsid(uint64_t pid)
{
    if (pid == 0)
        pid = get_current_pid();

    process_info_t *p = processes_info_ptr[pid];
    if (!p)
        return -ESRCH;

    return p->sid;
}
uint64_t sys_tcsetpgrp(uint64_t fd_id, uint64_t pgrp)
{
    if (fd_id > MAX_FD)
        return -100;
    uint64_t pid = get_current_pid();
    process_info_t *proc = processes_info_ptr[pid];
    struct file_inner *fd = &((struct file_inner *)proc->fd_addr)[fd_id];
    if (!fd)
        return -EBADF;
    if (fd->type != FILE_TTY)
        return -ENOTTY; // return error;
    uint64_t tty_id = fd->res_id;
    struct tty *tty = tty_global[tty_id];
    // validate...
    bool is_there_any_process_has_this_pgid = false;
    for (uint64_t i = 0; i < MAX_PROCESSES_COUNT; i++)
    {
        if (processes_info_ptr[i] && processes_info_ptr[i]->pgid == pgrp)
        {
            is_there_any_process_has_this_pgid = true;
            if (processes_info_ptr[i]->sid != proc->sid)
                return -EPERM;
            break;
        };
    };
    tty->foreground_pgid = pgrp;
    return 0;
}
uint64_t dup2(uint64_t oldfd, uint64_t newfd)
{
    process_info_t *proc = processes_info_ptr[get_current_pid()];
    struct file_inner *fds = (struct file_inner *)proc->fd_addr;
    // Validate range
    if (oldfd >= MAX_FD_INNER || newfd >= MAX_FD_INNER)
        return -EBADF; // EBADF

    // If oldfd == newfd → just return newfd
    if (oldfd == newfd)
        return newfd;

    
    if (fds[oldfd].type == FILE_NONE || fds[oldfd].global_id == 0)
        return -EBADF; // EBADF

    // If newfd is already open, close it first
    if (fds[newfd].type == FILE_NONE)
        sys_close(newfd);

    memcpy((uint8_t *)&fds[newfd], (uint8_t *)&fds[oldfd], sizeof(struct file_inner));
    return newfd;
}
uint64_t sys_pipe(int *pipefd_user_ptr)
{
    process_info_t *proc = processes_info_ptr[get_current_pid()];
    struct file_inner *fds = (struct file_inner *)proc->fd_addr;

    
    uint64_t pipe_id = get_a_free_slot((uint64_t)&pipe_global, 0, MAX_PIPES_COUNT);
    pipe_global[pipe_id] = (struct pipe *)get_a_free_block_addr();
    erase_block((uint64_t *)pipe_global[pipe_id]);

    
    uint64_t fd_r = get_a_free_slot_with_size(proc->fd_addr, 0, MAX_FD_INNER, sizeof(struct file_inner));
    uint64_t fd_w = get_a_free_slot_with_size(proc->fd_addr, fd_r + 1, MAX_FD_INNER, sizeof(struct file_inner));

    pipefd_user_ptr[0] = fd_r;
    pipefd_user_ptr[1] = fd_w;
    // fds + fd_r = (struct file_inner *)get_a_free_block_addr();
    // fds[fd_w] = (struct file_inner *)get_a_free_block_addr();
    
    erase_mem8((uint64_t)&fds[fd_r], sizeof(struct file_inner));
    erase_mem8((uint64_t)&fds[fd_w], sizeof(struct file_inner));

    fds[fd_r].type = FILE_PIPE;
    fds[fd_r].res_id = pipe_id;
    fds[fd_r].open_flags = O_RDONLY;

    fds[fd_w].type = FILE_PIPE;
    fds[fd_w].res_id = pipe_id;
    fds[fd_w].open_flags = O_WRONLY;

    return 0;
}
uint64_t sys_pipe2(int *pipefd_user_ptr, uint64_t flags)
{
    return sys_pipe(pipefd_user_ptr);
}
#endif // SYSCALL2_H
