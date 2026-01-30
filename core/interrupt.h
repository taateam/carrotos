#ifndef interrupt_H
#define interrupt_H

#include <stdint.h>
#include <stdbool.h>
extern void *idt_table;
extern uint64_t processes_info_lock;
extern uint64_t threads_info_lock;
extern uint64_t memory_map_lock;
extern void isr_keyboard_wrapper();
extern void isr_mouse_wrapper();
extern void isr_e1000_wrapper();
extern void xsave_context();
extern void xload_context();
#define SIGKILL 9
#define SIGSTOP 19
typedef struct
{
    volatile int locked;
} spinlock_t;
void spin_lock_init(spinlock_t *lock)
{
    lock->locked = 0;
}
void spin_lock(uint64_t *lock)
{
    uint64_t tmp = 1;
    while (1)
    {
        asm volatile(
            "xchg %0, %1"
            : "+r"(tmp), "+m"(*lock)
            :
            : "memory");
        if (tmp == 0) 
            return;
        tmp = 1; 
    }
}
void spin_unlock(uint64_t *lock_addr)
{
    uint64_t *lock = (uint64_t *)lock_addr;
    *lock = 0;
}
typedef struct
{
    uint16_t offset_low;  // bits 0..15
    uint16_t selector;    // segment selector (usually 0x08)
    uint8_t ist;          // bits 0..2 = IST index, 3..7 = 0
    uint8_t type_attr;    // type, DPL, P
    uint16_t offset_mid;  // bits 16..31
    uint32_t offset_high; // bits 32..63
    uint32_t zero;        // reserved
} __attribute__((packed)) idt_entry_t;
void register_interrupt_handler(int n, void *handler, uint8_t ist, uint8_t flags)
{
    uint64_t addr = (uint64_t)handler;
    idt_entry_t *idt = (idt_entry_t *)(&idt_table);
    idt[n].offset_low = addr & 0xFFFF;
    idt[n].selector = 0x08; // kernel CS
    idt[n].ist = ist & 0x7;
    idt[n].type_attr = flags; // e.g. 0x8E
    idt[n].offset_mid = (addr >> 16) & 0xFFFF;
    idt[n].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[n].zero = 0;
};
void handle_slepping_thread()
{
    // if (threads_info_lock1)
    //     return;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        thread_info_t *curr = threads_info_ptr[i];
        if ((uint64_t)curr == 0)
            continue;
        if (curr->state == THREAD_BLOCKED_OTHER && _later(get_time_us(), curr->wakeup_time))
            curr->state = THREAD_READY;
    }
}
uint64_t register_a_thread(uint64_t pid, uint64_t entry_point_addr);
void end_process(uint64_t pid, uint8_t return_code);
void stop_process(uint64_t pid);
uint64_t handle_signal_process(uint64_t pid)
{
    process_info_t *proc = processes_info_ptr[pid];
    if (proc->state == PROCESS_HANDLING_SIGNAL)
        return 0;
    if (proc->signal == SIGKILL)
    {
        end_process(pid, -1);
        return 0;
    }
    if (proc->signal == SIGSTOP)
    {
        stop_process(pid);
        return 0;
    }
    uint8_t sig = proc->signal;
    // process_info_t *proc = processes_info_ptr[pid];
    spin_lock(&processes_info_lock);
    uint64_t handler_tid = register_a_thread(pid, proc->sig_handlers[sig]);
    threads_info_ptr[handler_tid]->state = THREAD_READY;
    proc->state = PROCESS_HANDLING_SIGNAL;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (threads_info_ptr[i] && threads_info_ptr[i]->pid == pid && handler_tid != i)
            threads_info_ptr[i]->state = THREAD_WAITING_SIGNAL;
    }
    // tmp1[tid]->signal = sig;
    spin_unlock(&processes_info_lock);
    return handler_tid;
}
uint64_t handle_signal_thread(uint64_t tid)
{
    thread_info_t *thread = threads_info_ptr[tid];
    uint64_t pid = thread->pid;
    if (thread->signal == 0)
        return 0;
    uint8_t sig = thread->signal;
    process_info_t *proc = processes_info_ptr[pid];
    spin_lock(&processes_info_lock);
    uint64_t handler_tid = register_a_thread(pid, proc->sig_handlers[sig]);
    threads_info_ptr[handler_tid]->state = THREAD_READY;
    proc->state = PROCESS_HANDLING_SIGNAL;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (threads_info_ptr[i]->pid == pid)
            threads_info_ptr[i]->state = THREAD_WAITING_SIGNAL;
    }
    // tmp1[tid]->signal = sig;
    spin_unlock(&processes_info_lock);
    return handler_tid;
}
extern void end_thread(uint64_t tid, uint64_t return_code);
extern void gsbase_restore(uint64_t gs_base);
uint64_t timer_tick_from_idle(uint64_t *old_rsp)
{
    spin_lock(&threads_info_lock);
    current_tick++;
    uint64_t current_thread_id = 0;
    thread_info_t *curr = threads_info_ptr[current_thread_id];

    //===============================log
    // uint64_t core_id = get_apic_id();
    // echo(" ");
    // echoi(core_id);
    // // echo (".");
    // // echoi (current_tick);
    // echo(":");
    // echo("i");
    // echo("-");
    //==================================
    // handle sleepping thread
    handle_slepping_thread();
    
    // int tmp_tid = current_thread_id + 1;
    uint64_t new_pid = 0;
    uint64_t new_tid = 0;
    // spin_lock(&threads_info_lock1);
    for (uint64_t i = 1; i < MAX_THREADS_COUNT; i++)
    {
        uint64_t idx = (current_thread_id + i) % MAX_THREADS_COUNT;
        if (idx == 0)
            continue;
        uint64_t tmp = (uint64_t)(&threads_info_ptr[idx]);
        thread_info_t *new_thread_info = threads_info_ptr[idx];
        if ((uint64_t)new_thread_info == 0)
            continue;
        // thread_state_t aaaa = new_thread_info->state;
        if (new_thread_info->state == THREAD_READY)
        {
            new_tid = idx;
            break;
        }
    }
    if (new_tid == 2 && current_thread_id == 0)
    {
        int a = 1 + 1;
    }
    if (new_tid > 0)
    {
        // echoi(new_tid);
        thread_info_t *new_thread_info = threads_info_ptr[new_tid];
        new_thread_info->state = THREAD_RUNNING;
        // set gs:16
        new_pid = new_thread_info->pid;
        __asm__ volatile("movq %0, %%gs:8" ::"r"(new_thread_info->tid));
        __asm__ volatile("movq %0, %%gs:16" ::"r"(new_thread_info->kernel_stack_addr));
        // __asm__ volatile("movq %0, %%rbx" ::"r"(&new_thread_info->context));
        // __asm__ volatile("movq %0, %%rcx" ::"r"(new_pid));
        // __asm__ volatile("movq %0, %%rdx" ::"r"(processes_cr3_ptr[new_pid] - HM));
        if (new_thread_info->context_type > CONTEXT_TYPE_ALU)
            xload_context(&new_thread_info->context.xsave_area);
        // spin_unlock(&threads_info_lock1);
        gsbase_restore(new_thread_info->context.gs_base);
        return new_tid;
    }
    
    // echo("i");
    uint64_t tmp = 0;
    __asm__ volatile("movq %0, %%gs:8" ::"r"(tmp));
    __asm__ volatile("movq %0, %%gs:16" ::"r"(tmp)); // idle don't use stack, don't call syscall;
    // __asm__ volatile("movq %0, %%rbx" ::"r"(&threads_info_ptr[0]->context));
    // __asm__ volatile("movq %0, %%rcx" ::"r"(new_pid));
    // __asm__ volatile("movq %0, %%rdx" ::"r"(&page_table_l4));
    return 0;
}

uint64_t timer_tick_from_non_idle(uint64_t *old_rsp)
{
    spin_lock(&threads_info_lock);
    current_tick++;
    uint64_t current_thread_id = get_current_thread_id();
    thread_info_t *curr = threads_info_ptr[current_thread_id];

    //===============================log
    // uint64_t core_id = get_apic_id();
    // echo(" ");
    // echoi(core_id);
    // // echo (".");
    // // echoi (current_tick);
    // echo(":");
    // echoi(current_thread_id);
    // echo("-");
    //==================================
    
    uint64_t *stack = old_rsp;
    bool from_user = (stack[16] & 0x3) == 0x3;
    curr->context.rax = stack[0];
    curr->context.rcx = stack[1];
    curr->context.rdx = stack[2];
    curr->context.rbx = stack[3];
    curr->context.rbp = stack[4];
    curr->context.rsi = stack[5];
    curr->context.rdi = stack[6];
    curr->context.r8 = stack[7];
    curr->context.r9 = stack[8];
    curr->context.r10 = stack[9];
    curr->context.r11 = stack[10];
    curr->context.r12 = stack[11];
    curr->context.r13 = stack[12];
    curr->context.r14 = stack[13];
    curr->context.r15 = stack[14];

    curr->context.rip = stack[15];
    if (stack[15] == 1)
    {
        int a = 1 + 1;
    }
    curr->context.cs = stack[16];
    curr->context.rflags = stack[17];
    if (curr->context_type > CONTEXT_TYPE_ALU)
        xsave_context(&curr->context.xsave_area);
    if ((stack[16] & 0x3) == 0x3)
    {
        // From user space
        curr->context.rsp = stack[18];
        // curr->context.ss = stack[19];
    }
    else
    {
        
        // asm volatile("mov %%rsp, %0" : "=r"(curr->context.rsp));
        curr->context.rsp = (uint64_t)old_rsp + (20 * 8);
        asm volatile("mov %%ss, %0" : "=r"(curr->context.ss));
    }
    if (curr->context.cs > 0x10)
    {
        curr->context.ss = 0x1B;
        curr->context.cs = 0x23;
    }
    else
    {
        curr->context.ss = 0x10;
        curr->context.cs = 0x08;
    }
    __asm__ volatile("movq %%gs:24, %0" : "=r"(curr->context.gs24));
    // if (curr->context.ss == 0)
    // {
    //     int a = 1 + 1;
    // }
    // handle sleepping thread
    handle_slepping_thread();
    
    // int tmp_tid = current_thread_id + 1;
    uint64_t new_pid = 0;
    uint64_t new_tid = 0;
    // spin_lock(&threads_info_lock1);
    for (uint64_t i = 1; i < MAX_THREADS_COUNT; i++)
    {
        uint64_t idx = (current_thread_id + i) % MAX_THREADS_COUNT;
        if (idx == 0)
            continue;
        uint64_t tmp = (uint64_t)(&threads_info_ptr[idx]);
        thread_info_t *new_thread_info = threads_info_ptr[idx];
        if ((uint64_t)new_thread_info == 0)
            continue;
        // thread_state_t aaaa = new_thread_info->state;
        if (new_thread_info->state == THREAD_READY)
        {
            new_tid = idx;
            break;
        }
    }
    if (new_tid == 2 && current_thread_id == 0)
    {
        int a = 1 + 1;
    }
    if (curr->state == THREAD_ENDED_BUT_NOT_CLEANED)
    {
        // echo("e");
        end_thread(current_thread_id, -1);
    }
    if (new_tid > 0)
    {
        // echoi(new_tid);
        if (curr->state == THREAD_RUNNING)
            curr->state = THREAD_READY;
        thread_info_t *new_thread_info = threads_info_ptr[new_tid];
        new_thread_info->state = THREAD_RUNNING;
        // set gs:16
        new_pid = new_thread_info->pid;
        __asm__ volatile("movq %0, %%gs:8" ::"r"(new_thread_info->tid));
        __asm__ volatile("movq %0, %%gs:16" ::"r"(new_thread_info->kernel_stack_addr));
        __asm__ volatile("movq %0, %%gs:24" ::"r"(new_thread_info->context.gs24));
        // __asm__ volatile("movq %0, %%rbx" ::"r"(&new_thread_info->context));
        // __asm__ volatile("movq %0, %%rcx" ::"r"(new_pid));
        // __asm__ volatile("movq %0, %%rdx" ::"r"(processes_cr3_ptr[new_pid] - HM));
        if (new_thread_info->context_type > CONTEXT_TYPE_ALU)
            xload_context(&new_thread_info->context.xsave_area);
        // spin_unlock(&threads_info_lock1);
        gsbase_restore(new_thread_info->context.gs_base);
        return new_tid;
    }
    
    if (curr->state == THREAD_RUNNING)
    {
        // echo("c");
        new_pid = curr->pid;
        // __asm__ volatile("movq %0, %%rbx" ::"r"(&curr->context));
        // __asm__ volatile("movq %0, %%rcx" ::"r"(new_pid));
        // __asm__ volatile("movq %0, %%rdx" ::"r"(processes_cr3_ptr[new_pid] - HM));
        // spin_unlock(&threads_info_lock1);
        return current_thread_id;
    }
    if (curr->state != THREAD_READY)
    {
        // echo("i");
        uint64_t tmp = 0;
        new_pid = 0;
        __asm__ volatile("movq %0, %%gs:8" ::"r"(tmp));
        __asm__ volatile("movq %0, %%gs:16" ::"r"(tmp)); // idle don't use stack, don't call syscall;
        __asm__ volatile("movq %0, %%gs:24" ::"r"(tmp));
        // __asm__ volatile("movq %0, %%rbx" ::"r"(&threads_info_ptr[0]->context));
        // __asm__ volatile("movq %0, %%rcx" ::"r"(new_pid));
        // __asm__ volatile("movq %0, %%rdx" ::"r"(&page_table_l4));
        gsbase_restore(0);
        return 0;
    }
}

uint64_t timer_tick1_counter = 0;
// this is for syscall or kernel threads want to yeild()
// idle won't call this
uint64_t timer_tick1(uint64_t *old_rsp)
{
    // echo("\ntimer_tick1: ");
    // char count_str[64];
    // num_to_str(timer_tick1_counter, count_str);
    // echo(count_str);
    // timer_tick1_counter++;
    spin_lock((uint64_t *)&threads_info_lock);
    current_tick++;
    uint64_t current_thread_id = get_current_thread_id();
    thread_info_t *curr = threads_info_ptr[current_thread_id];

    
    uint64_t *stack = old_rsp;
    bool from_user = (stack[16] & 0x3) == 0x3;
    curr->context.rax = stack[0];
    curr->context.rcx = stack[1];
    curr->context.rdx = stack[2];
    curr->context.rbx = stack[3];
    curr->context.rbp = stack[4];
    curr->context.rsi = stack[5];
    curr->context.rdi = stack[6];
    curr->context.r8 = stack[7];
    curr->context.r9 = stack[8];
    curr->context.r10 = stack[9];
    curr->context.r11 = stack[10];
    curr->context.r12 = stack[11];
    curr->context.r13 = stack[12];
    curr->context.r14 = stack[13];
    curr->context.r15 = stack[14];

    curr->context.rip = stack[15];
    if (stack[15] > 0x400000400000)
    {
        int a = 1 + 1;
    }
    curr->context.cs = stack[16];
    curr->context.rflags = stack[17];
    if (curr->context_type > CONTEXT_TYPE_ALU)
        xsave_context(&curr->context.xsave_area);
    if ((stack[16] & 0x3) == 0x3)
    {
        // From user space
        curr->context.rsp = stack[18];
        // curr->context.ss = stack[19];
    }
    else
    {
        
        // asm volatile("mov %%rsp, %0" : "=r"(curr->context.rsp));
        curr->context.rsp = (uint64_t)old_rsp + (20 * 8);
        asm volatile("mov %%ss, %0" : "=r"(curr->context.ss));
    }
    if (curr->context.cs > 0x10)
    {
        curr->context.ss = 0x1B;
        curr->context.cs = 0x23;
    }
    else
    {
        curr->context.ss = 0x10;
        curr->context.cs = 0x08;
    }
    __asm__ volatile("movq %%gs:24, %0" : "=r"(curr->context.gs24));
    // handle sleepping thread
    handle_slepping_thread();
    
    // int tmp_tid = current_thread_id + 1;
    uint64_t new_pid = 0;
    uint64_t new_tid = 0;
    for (uint64_t i = 1; i < MAX_THREADS_COUNT; i++)
    {
        uint64_t idx = (current_thread_id + i) % MAX_THREADS_COUNT;
        if (idx == 0)
            continue;
        uint64_t tmp = (uint64_t)(&threads_info_ptr[idx]);
        thread_info_t *new_thread_info = threads_info_ptr[idx];
        if ((uint64_t)new_thread_info == 0)
            continue;
        // thread_state_t aaaa = new_thread_info->state;
        if (new_thread_info->state == THREAD_READY)
        {
            new_tid = idx;
            break;
        }
    }
    if (curr->state == THREAD_ENDED_BUT_NOT_CLEANED)
    {
        // echo("e");
        end_thread(current_thread_id, -1);
    }
    if (new_tid > 1)
    {
        if (curr->state == THREAD_RUNNING)
            curr->state = THREAD_READY;
        thread_info_t *new_thread_info = threads_info_ptr[new_tid];
        new_thread_info->state = THREAD_RUNNING;
        // set gs:16
        new_pid = new_thread_info->pid;
        __asm__ volatile("movq %0, %%gs:8" ::"r"(new_thread_info->tid));
        __asm__ volatile("movq %0, %%gs:16" ::"r"(new_thread_info->kernel_stack_addr));
        __asm__ volatile("movq %0, %%gs:24" ::"r"(new_thread_info->context.gs24));
        // __asm__ volatile("movq %0, %%rbx" ::"r"(&new_thread_info->context));
        // __asm__ volatile("movq %0, %%rcx" ::"r"(new_pid));
        // __asm__ volatile("movq %0, %%rdx" ::"r"(processes_cr3_ptr[new_pid]));
        if (new_thread_info->context_type > CONTEXT_TYPE_ALU)
            xload_context(&new_thread_info->context.xsave_area);
        gsbase_restore(new_thread_info->context.gs_base);
        return new_tid;
    }
    
    uint64_t tmp = 0;
    new_pid = 0;
    __asm__ volatile("movq %0, %%gs:8" ::"r"(tmp));
    __asm__ volatile("movq %0, %%gs:16" ::"r"(tmp)); // idle don't use stack, don't call syscall;
    __asm__ volatile("movq %0, %%gs:24" ::"r"(tmp));
    // __asm__ volatile("movq %0, %%rbx" ::"r"(&threads_info_ptr[0]->context));
    // __asm__ volatile("movq %0, %%rcx" ::"r"(new_pid));
    // __asm__ volatile("movq %0, %%rdx" ::"r"(page_table_l4));
    gsbase_restore(0);
    return 0;
}

void cli(void)
{
    asm volatile("cli");
}

void sti(void)
{
    asm volatile("sti");
}
void scheduler_yeild()
{
    // asm volatile("cli");
    tick_interrupt_handler1();
    // asm volatile("sti");
}
extern uint64_t syscall_dispatch(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                 uint64_t arg4, uint64_t arg5, uint64_t arg6);
extern void panic(char input[]);
void handle_syscall_interrupt(uint64_t *stack_ptr)
{
    uint64_t tid;
    __asm__ volatile("mov %%gs:8, %0" : "=r"(tid));
    thread_info_t *thread = &((thread_info_t *)threads_info_ptr)[tid];

    
    uint64_t pid = thread->pid;
    process_info_t *process = 0x0;
    for (int i = 0; i < MAX_PROCESSES_COUNT; i++)
    {
        if (((process_info_t *)processes_info_ptr)[i].pid == pid)
        {
            process = processes_info_ptr[i];
            break;
        }
    }

    if (process == 0x0)
    {
        
        panic("no process to switch!");
    }

    uint64_t cr3 = processes_cr3_ptr[pid] - HM; // Switch CR3 sang process address space
    __asm__ volatile("mov %0, %%cr3" ::"r"(cr3) : "memory");
    // switch (syscall_id)
    // {
    // case SYSCALL_PRINT:
    //     echo((const char *)thread->context.rdi);
    //     break;

    // case SYSCALL_HLT:
    //     __asm__ volatile("hlt");
    //     break;

    // default:
    //     // syscall unknown
    //     break;
    // };
    // params;
    uint64_t syscall_id = stack_ptr[0];
    uint64_t arg1 = stack_ptr[5]; // rdi
    uint64_t arg2 = stack_ptr[4]; // rsi
    uint64_t arg3 = stack_ptr[3]; // rdx
    uint64_t arg4 = stack_ptr[9]; // r10
    uint64_t arg5 = stack_ptr[6]; // r8
    uint64_t arg6 = stack_ptr[7]; // r9
    asm volatile("mov %0, %%r10" ::"r"(syscall_id) : "r10");
    syscall_dispatch(arg1, arg2, arg3, arg4, arg5, arg6);
    return;
}

void handle_slepping_threads()
{
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        thread_info_t thread = ((thread_info_t *)&threads_info_ptr)[i];
        if ((thread.state == THREAD_BLOCKED_OTHER || thread.state == THREAD_BLOCKED_FUTEX) && thread.wakeup_time.tv_sec && _later(thread.wakeup_time, get_time_us()))
        {
            thread.state = THREAD_READY;
        };
    };
}
extern uint8_t sse_mode;
extern void enable_sse();
void switch_context_type()
{
    thread_info_t *thread = threads_info_ptr[get_current_thread_id()];
    if (!thread)
        return;
    if (!sse_mode)
    {
        sse_mode = 1;
        enable_sse();
    }
    if (thread->context_type == CONTEXT_TYPE_ALU)
        thread->context_type = CONTEXT_TYPE_SSE;
    else
        thread->signal = 4; // sigill;
}
void seg_fault_signal()
{
    thread_info_t *thread = threads_info_ptr[get_current_thread_id()];
    if (!thread)
        return;
    thread->signal = 11;
}
void float_error_signal()
{
    thread_info_t *thread = threads_info_ptr[get_current_thread_id()];
    if (!thread)
        return;
    thread->signal = 8;
}
// #define KBD_BUF_SIZE 256
#endif
