#ifndef SIGNAL_H
#define SIGNAL_H

#define MAX_SIGNAL 63

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGRTMIN 32
#define SIGRTMAX 64

union sigval
{
    int sival_int;
    void *sival_ptr;
};
struct signal
{
    int si_signo; 
    int si_errno; 
    int si_code;  

    union
    {
        struct
        {
            int32_t si_pid;
            int32_t si_uid;
        } kill;
        struct
        {
            int32_t si_pid;
            int32_t si_uid;
            union sigval si_value;
        } sigqueue;
        struct
        {
            void *si_addr;
            uint16_t si_addr_lsb;
            uint16_t _pad[3];
        } fault;
        struct
        {
            void *si_addr;
            int32_t si_trapno;
            uint32_t _pad;
        } trap;
        struct
        {
            int32_t si_pid;
            int32_t si_uid;
            int32_t si_timerid;
            union sigval si_value;
        } timer;
        struct
        {
            void *ptr;
            int64_t val;
        } generic;
    } _sifields;
} __attribute__((packed));

static inline void mask_a_signal(uint64_t *mask, uint8_t signal_num)
{
    if (signal_num > MAX_SIGNAL)
        return;
    *mask |= (1ULL << signal_num);
}


static inline void unmask_a_signal(uint64_t *mask, uint8_t signal_num)
{
    if (signal_num > MAX_SIGNAL)
        return;
    *mask &= ~(1ULL << signal_num);
}


// action = 0 -> unblock, action = 1 -> block
static inline void do_mask_a_signal(uint64_t *mask, uint8_t signal_num, bool action)
{
    if (action)
        mask_a_signal(mask, signal_num);
    else
        unmask_a_signal(mask, signal_num);
}
bool is_signal_masked(uint8_t signal_num, uint64_t signal_mask)
{
    return (signal_mask & (1ULL << signal_num)) != 0;
}
uint64_t send_signal_to_process(uint64_t pid, uint8_t sig, struct signal *info)
{
    if (sig > MAX_SIGNAL)
        return -EINVAL;
    bool found_handling_thread = false;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        thread_info_t *tmp_thread = threads_info_ptr[i];
        if (tmp_thread->pid != pid)
            continue;
        if (is_signal_masked(sig, tmp_thread->signal_mask))
            continue;
        found_handling_thread = true;
        tmp_thread->signal = sig;
        break;
    }
    if (!found_handling_thread)
        end_process(pid, SIGKILL);
    else
    {
        processes_info_ptr[pid]->signal = sig;
        handle_signal_process(pid);
    }
}
uint64_t send_signal_to_thread(uint64_t pid, uint64_t tid, uint8_t sig, struct signal *info)
{
    if (sig > MAX_SIGNAL)
        return -EINVAL;

    thread_info_t *t = threads_info_ptr[tid];
    if (!t || t->pid != pid)
        return -ESRCH; // no such thread

    
    if (is_signal_masked(sig, t->signal_mask))
        return -EACCES; 

    
    t->signal = sig;
    handle_signal_thread(tid);

    
    // if (info)
    

    return 0; 
};
#endif // SIGNAL_H
