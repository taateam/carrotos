bits 64
section .text
global init_pit

;========================================================================
init_pit:
    ;
    mov al, 0x36        
    out 0x43, al        

    ; 1193182 / 1000Hz = ~1193
    mov ax, 1193
    out 0x40, al         ; Low byte
    mov al, ah
    out 0x40, al         ; High byte
    
    ret

;========================================================================
extern idt_table
extern idt_ptr
extern timer_tick_from_idle
extern timer_tick_from_non_idle
extern handle_syscall_interrupt
extern swap_gs_if_from_user_mode

global register_interrupt_handlers
register_interrupt_handlers:

    ;
    mov ax, 16*256 - 1
    mov rbx, idt_ptr
    mov [rbx], ax     ; limit
    mov rcx, idt_ptr
    add rcx, 2
    mov rax, idt_table
    mov [rcx], rax                ; base

    ; Load IDT
    lidt [rbx]

    ret

extern threads_info_ptr
extern processes_cr3_ptr
extern processes_info_ptr
extern page_table_l4
tick_interrupt_handler_from_idle:
    ;=====mov rsp==============
    mov rax, qword [gs:32]
    mov rsp, rax
    ;==========================
    ;
    mov rdi, rsp
    call timer_tick_from_idle  
    ;calculate; 
    ;rax = thread_id, rbx = context of thread_id, rcx = process_id, rdx: cr3 of parent process.
    mov rcx, threads_info_ptr
    mov rdi, rax
    imul rdi, 8
    add rcx, rdi
    mov rcx, [rcx]
    mov rbx, rcx
    mov rcx, [rcx]
    add rbx, 64; 0x38
    cmp rcx, 1
    jne .user_thread
    .kernel_thread:
        mov rdx, page_table_l4; kernel_thread use same kernel cr3
        jmp .end_if
    .user_thread:
        mov rdx, processes_cr3_ptr
        mov rdi, rcx
        imul rdi, 8
        add rdx, rdi
        mov rdx, [rdx]        
        mov rsi, 0x400000000000
        sub rdx, rsi
        jmp .end_if
    .end_if:
    ;

    ;add rsp, 15 * 8

    ; Restore general-purpose registers
    mov r15, [rbx + 0x00]
    mov r14, [rbx + 0x08]
    mov r13, [rbx + 0x10]
    mov r12, [rbx + 0x18]
    mov r11, [rbx + 0x20]
    mov r10, [rbx + 0x28]
    mov r9,  [rbx + 0x30]
    mov r8,  [rbx + 0x38]
    mov rdi, [rbx + 0x40]
    mov rsi, [rbx + 0x48]
    mov rbp, [rbx + 0x50]
   
    ; switch page
    cmp rcx, 1
    ja .if_user_thread
    .if_kernel_thread:
        ;
        mov rsp, [rbx + 0x90]    ;
        ;
        push qword [rbx + 0x98]  ; SS
        push qword [rbx + 0x90]  ; RSP
        push qword [rbx + 0x88]  ; RFLAGS
        push qword [rbx + 0x80]  ; CS
        push qword [rbx + 0x78]  ; RIP
        mov cr3, rdx
        jmp .end_if_thread_type
    .if_user_thread:
        ;
        push qword [rbx + 0x98]  ; SS
        push qword [rbx + 0x90]  ; RSP
        push qword [rbx + 0x88]  ; RFLAGS
        push qword [rbx + 0x80]  ; CS
        push qword [rbx + 0x78]  ; RIP
        ; switch cr3
        mov cr3, rdx
        jmp .end_if_thread_type
    .end_if_thread_type:
    
    ; end interupt
    ;mov al, 0x20
    ;out 0x20, al
    mov rax, 0x400000000000
    mov rcx, 0xFEE000B0; lapic mmio
    add rax, rcx
    mov dword [rax], 0
    ; continue restore general-purpose registers
    mov rdx, [rbx + 0x60]
    mov rcx, [rbx + 0x68]
    ; rax and rbx
    mov rax, [rbx + 0x70]
    mov rbx, [rbx + 0x58]   ;rbx is context
    ; sti
    call swap_gs_if_from_user_mode
    push rbx
    extern threads_info_lock
    mov rbx, threads_info_lock
    mov qword [rbx], 0
    pop rbx
    .iretq:
        iretq
    
tick_interrupt_handler_from_non_idle:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rbp
    push rbx
    push rdx
    push rcx
    push rax

    ;
    mov rdi, rsp
    call timer_tick_from_non_idle  
    ;calculate; 
    ;rax = thread_id, rbx = context of thread_id, rcx = process_id, rdx: cr3 of parent process.
    mov rcx, threads_info_ptr
    mov rdi, rax
    imul rdi, 8
    add rcx, rdi
    mov rcx, [rcx]
    mov rbx, rcx
    mov rcx, [rcx]
    add rbx, 64; 0x38
    cmp rcx, 1
    jne .user_thread
    .kernel_thread:
        mov rdx, page_table_l4; kernel_thread use same kernel cr3
        jmp .end_if
    .user_thread:
        mov rdx, processes_cr3_ptr
        mov rdi, rcx
        imul rdi, 8
        add rdx, rdi
        mov rdx, [rdx]        
        mov rsi, 0x400000000000
        sub rdx, rsi
        jmp .end_if
    .end_if:
    ;

    ;add rsp, 15 * 8

    ; Restore general-purpose registers
    mov r15, [rbx + 0x00]
    mov r14, [rbx + 0x08]
    mov r13, [rbx + 0x10]
    mov r12, [rbx + 0x18]
    mov r11, [rbx + 0x20]
    mov r10, [rbx + 0x28]
    mov r9,  [rbx + 0x30]
    mov r8,  [rbx + 0x38]
    mov rdi, [rbx + 0x40]
    mov rsi, [rbx + 0x48]
    mov rbp, [rbx + 0x50]
   
    ; switch page
    cmp rcx, 1
    ja .if_user_thread
    .if_kernel_thread:
        ;
        mov rsp, [rbx + 0x90]    ;
        ;
        push qword [rbx + 0x98]  ; SS
        push qword [rbx + 0x90]  ; RSP
        push qword [rbx + 0x88]  ; RFLAGS
        push qword [rbx + 0x80]  ; CS
        push qword [rbx + 0x78]  ; RIP
        mov cr3, rdx
        jmp .end_if_thread_type
    .if_user_thread:
        ;
        push qword [rbx + 0x98]  ; SS
        push qword [rbx + 0x90]  ; RSP
        push qword [rbx + 0x88]  ; RFLAGS
        push qword [rbx + 0x80]  ; CS
        push qword [rbx + 0x78]  ; RIP
        ; switch cr3
        mov cr3, rdx
        jmp .end_if_thread_type
    .end_if_thread_type:
    
    ; end interupt
    ;mov al, 0x20
    ;out 0x20, al
    mov rax, 0x400000000000
    mov rcx, 0xFEE000B0; lapic mmio
    add rax, rcx
    mov dword [rax], 0
    ; continue restore general-purpose registers
    mov rdx, [rbx + 0x60]
    mov rcx, [rbx + 0x68]
    ; rax and rbx
    mov rax, [rbx + 0x70]
    mov rbx, [rbx + 0x58]   ;rbx is context
    ; sti
    push rbx
    extern threads_info_lock
    mov rbx, threads_info_lock
    mov qword [rbx], 0
    pop rbx
    call swap_gs_if_from_user_mode
    .iretq:
        iretq
    
global tick_interrupt_handler
tick_interrupt_handler:
    call swap_gs_if_from_user_mode
    cli
    ;==========================
    push rax
    mov rax, qword [gs:8]
    cmp rax, 0
    je .idle
    .non_idle:
        pop rax
        jmp tick_interrupt_handler_from_non_idle
    .idle:
        pop rax
        jmp tick_interrupt_handler_from_idle
    .endif:
    ;these tick_interrupt_handlers won't return, they iretq
    ;==========================

extern timer_tick1
global tick_interrupt_handler1
tick_interrupt_handler1:
    call swap_gs_if_from_user_mode
    cli
    xor rax, rax
    ;mov rax, ss
    ;push rax
    push 0x10; ss
    mov rax, rsp
    add rax, 8
    push rax
    ;push qword [rsp - 16]
    pushfq ;  rflags
    ;mov al, ss
    ;push rax
    push 0x8 ;cs
    mov rax, retx
    push rax;rip
    jmp retx.not_retx
    retx:
        ;pop rax
        ;pop rax
        ;pop rax
        ;pop rax
        ;pop rax
        ret
    
    .not_retx:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rbp
    push rbx
    push rdx
    push rcx
    push rax

    ;
    mov rdi, rsp
    call timer_tick1 
    ;calculate; 
    ;rax = thread_id, rbx = context of thread_id, rcx = process_id, rdx: cr3 of parent process.
    mov rcx, threads_info_ptr
    mov rdi, rax
    imul rdi, 8
    add rcx, rdi
    mov rcx, [rcx]
    mov rbx, rcx
    mov rcx, [rcx]
    add rbx, 64; 0x38
    cmp rcx, 1
    jne .user_thread
    .kernel_thread1:
        mov rdx, page_table_l4; kernel_thread use same kernel cr3
        jmp .end_if
    .user_thread:
        mov rdx, processes_cr3_ptr
        mov rdi, rcx
        imul rdi, 8
        add rdx, rdi
        mov rdx, [rdx]        
        mov rsi, 0x400000000000
        sub rdx, rsi
        jmp .end_if
    .end_if:
    ;

    ; add rsp, 15 * 8

    ; Restore general-purpose registers
    mov r15, [rbx + 0x00]
    mov r14, [rbx + 0x08]
    mov r13, [rbx + 0x10]
    mov r12, [rbx + 0x18]
    mov r11, [rbx + 0x20]
    mov r10, [rbx + 0x28]
    mov r9,  [rbx + 0x30]
    mov r8,  [rbx + 0x38]
    mov rdi, [rbx + 0x40]
    mov rsi, [rbx + 0x48]
    mov rbp, [rbx + 0x50]
   
    ; switch page
    cmp rcx, 1
    jne .if_user_thread
    .if_kernel_thread:
        ;
        ;
        ;
        push qword [rbx + 0x98]  ; SS
        push qword [rbx + 0x90]  ; RSP
        push qword [rbx + 0x88]  ; RFLAGS
        push qword [rbx + 0x80]  ; CS
        push qword [rbx + 0x78]  ; RIP
        mov cr3, rdx
        jmp .end_if_thread_type
    .if_user_thread:
        ;
        push qword [rbx + 0x98]  ; SS
        push qword [rbx + 0x90]  ; RSP
        push qword [rbx + 0x88]  ; RFLAGS
        push qword [rbx + 0x80]  ; CS
        push qword [rbx + 0x78]  ; RIP
        ; switch cr3
        mov cr3, rdx
        jmp .end_if_thread_type
    .end_if_thread_type:
    
    ; end interupt
    ;mov al, 0x20
    ;out 0x20, al
    mov rax, 0x400000000000
    mov rcx, 0xFEE000B0; lapic mmio
    add rax, rcx
    mov dword [rax], 0
    ; continue restore general-purpose registers
    mov rdx, [rbx + 0x60]
    mov rcx, [rbx + 0x68]
    ; rax and rbx
    mov rax, [rbx + 0x70]
    mov rbx, [rbx + 0x58]   ;rbx is context
    ; sti
    push rbx
    extern threads_info_lock
    mov rbx, threads_info_lock
    mov qword [rbx], 0
    pop rbx
    call swap_gs_if_from_user_mode
    .iretq:
        iretq
    
global syscall_interrupt_handler
syscall_interrupt_handler:
    ;
    push rax
    push rbx
    push rcx      ;
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11      ;
    push r12
    push r13
    push r14
    push r15
    push rbp

    ;
    mov rdi, rsp
    call handle_syscall_interrupt

    ;
    pop rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    mov al, 0x20
    out 0x20, al;
    iretq

global xsleep
xsleep:
    hlt
    jmp xsleep

global dummy_interrupt_handler
dummy_interrupt_handler:
    ;
    in al, 0x60
    extern clear_screen
    call clear_screen
    ;sti
    mov al, 0x20
    out 0x20, al
    iretq

global dummy_interrupt_handler1
dummy_interrupt_handler1:
    ;
    extern X64
    call X64
    ;sti
    mov al, 0x20
    out 0x20, al
    iretq
    
global dummy_interrupt_handler2
dummy_interrupt_handler2:
    push rax
    mov al, 0x20
    out 0x20, al
    pop rax
    iretq
    
global enable_sse
enable_sse:
    mov eax, 1
    cpuid
    xor rax, rax
    cli
    ; 1. Enable FPU
    mov rax, cr0
    and rax, ~(1 << 2)       ; EM=0
    mov cr0, rax

    ; 2. Enable SSE + FXSAVE/FXRSTOR + SSE exceptions + OSXSAVE
    mov rax, cr4
    or rax, (1 << 9)         ; OSFXSR
    or rax, (1 << 10)        ; OSXMMEXCPT
    or rax, (1 << 18)        ; OSXSAVE
    mov cr4, rax

    ; 3. Enable AVX in XCR0
    xor rax, rax
    xor rdx, rdx
    mov ecx, 0               ; XCR0
    mov eax, (1<<0)|(1<<1)|(1<<2)  ; x87 + SSE + AVX
    xsetbv
    sti
    ret

section seed.text
align 4096
global begin_seed
begin_seed:
global seed_text_start
seed_text_start:
    ;mov rsp, 0x3ffffffff000
    ;mov rax, 0x1000
    ;jmp rax
    
    mov r8, seed_text_start
    sub r8, 0x1000
    mov rax, 1          ; syscall number 1
    mov rdi, 1          ; fd = 1 (stdout)
    mov r9, msg
    sub r9, r8
    mov rsi, r9         ;
    mov rdx, 13         ; len = 13

    ;ud2
    ;
    ;ud2
    ;syscall

    ;mov rax, 0x1000
    ;

jmp1:
    mov r9, jmp1
    sub r9, r8
    jmp r9
    
    ;mov rax, 0x1000
    ;jmp rax
    ;

msg:
    db "Hello, world", 0  ;
    ;ret
global seed_text_end
seed_text_end:

align 4096
section seed.data
global seed_data_start
seed_data_start:
    times 4096*4 db 0; resb 4096*4
global seed_data_end
seed_data_end:

align 4096
section seed.bss
global seed_bss_start
seed_bss_start:
    times 4096*4 db 0;resb 4096*4
global seed_bss_end
seed_bss_end:

align 4096
section seed.rodata
global seed_rodata_start
seed_rodata_start:
    times 4096*4 db 0;resb 4096*4
global seed_rodata_end
seed_rodata_end:

align 4096
section seed.heap
global seed_heap_start
seed_heap_start:
    times 4096*4 db 0;resb 4096*4
global seed_heap_end
seed_heap_end:

global end_seed
end_seed:

