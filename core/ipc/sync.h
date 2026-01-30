#ifndef SYNC_H
#define SYNC_H

uint64_t pause_thread_futex(uint64_t tid, uint32_t *futex_ptr, uint32_t expected_val, struct timespec *timeout)
{
    thread_info_t *thread = threads_info_ptr[tid];
    uint64_t pid = thread->pid;
    if (!is_paged(pid, (uint64_t)futex_ptr) || !is_paged(pid, (uint64_t)futex_ptr + 4))
        return -EFAULT;
    if (*futex_ptr != expected_val)
        return -EAGAIN;
    thread->state = THREAD_BLOCKED_FUTEX;
    if (timeout)
    {
        struct timeval timeout_v = timespec_to_timeval(*timeout);
        struct timeval now = get_time_us();
        struct timeval wt = add_time(now, timeout_v);
        memcpy((uint8_t *)&thread->wakeup_time, (uint8_t *)&wt, sizeof(struct timeval));
        thread->lock_time.tv_sec = now.tv_sec;
        thread->lock_time.tv_microsec = now.tv_microsec;
    }
    thread->sleep_flag_addr = (uint64_t)futex_ptr;
    scheduler_yeild();
    return 0;
}
uint64_t resume_thread_futex(uint32_t *futex_ptr)
{
    struct timeval earliest = get_time_us();
    bool found_thread = false;
    uint64_t resume_tid;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        thread_info_t *t = threads_info_ptr[i];
        if (!t)
            continue;
        if (t->pid == get_current_pid() && t->state == THREAD_BLOCKED_FUTEX && t->sleep_flag_addr == (uint64_t)futex_ptr)
        {
            found_thread = true;
            if (earlier(t->lock_time, earliest))
            {
                resume_tid = i;
                earliest.tv_sec = t->lock_time.tv_sec;
                earliest.tv_microsec = t->lock_time.tv_microsec;
            }
        }
    }
    if (!found_thread)
        return -EINVAL;
    else
    {
        threads_info_ptr[resume_tid]->state = THREAD_READY;
        threads_info_ptr[resume_tid]->sleep_flag_addr = 0;
        threads_info_ptr[resume_tid]->wakeup_time.tv_sec = 0;
        threads_info_ptr[resume_tid]->wakeup_time.tv_microsec = 0;
        threads_info_ptr[resume_tid]->lock_time.tv_sec = 0;
        threads_info_ptr[resume_tid]->lock_time.tv_microsec = 0;
    }
    return 0;
}
uint64_t move_futex(uint32_t *uaddr, uint32_t op, uint32_t *uaddr2, uint32_t val3)
{
    uint64_t pid = get_current_pid();
    if (!is_paged(pid, (uint64_t)uaddr) || !is_paged(pid, (uint64_t)uaddr + 4))
        return -EFAULT;
    uint64_t count = 0;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        thread_info_t *t = threads_info_ptr[i];
        if (t->pid == get_current_pid() && t->state == THREAD_BLOCKED_FUTEX && t->sleep_flag_addr == (uint64_t)uaddr)
        {
            t->sleep_flag_addr = (uint64_t)uaddr2;
            count++;
        }
    }
    if (*uaddr2 == val3)
        resume_thread_futex(uaddr2);
    return count;
}
#endif
