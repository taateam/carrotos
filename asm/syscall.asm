%define IA32_EFER      0xC0000080
%define IA32_LSTAR     0xC0000082
%define IA32_STAR      0xC0000081
%define IA32_FMASK     0xC0000084
%define IA32_KERNEL_GS_BASE  0xC0000102

%define GDT_KERNEL_CS  0x08    ;
%define GDT_USER_CS    0x18    ;

section .text
global setup_syscall
setup_syscall:
    ; Enable syscall in EFER
    mov ecx, IA32_EFER
    rdmsr
    or eax, 1
    wrmsr

    ; Set syscall entrypoint (LSTAR)
    mov ecx, IA32_LSTAR
    mov rax, entry_syscall
    shr rax, 0x20
    mov edx, eax
    mov rax, entry_syscall
    wrmsr

    ; STAR: segment for syscall/sysret (upper 32 bits)
    ; (Kernel_CS << 3) << 16 | (User_CS << 3)
    ; => ((0x08) << 3) << 16 | ((0x18) << 3)
    ; => 0x00080000 | 0x000000C0 = 0x000800C0
    mov ecx, IA32_STAR
    mov eax, 0                         ; bits 31..0: ignored
    mov edx, 0x00100008           ; user_cs << 16 | kernel_cs
    wrmsr

    ; Mask IF (bit 9), TF (bit 8), v.v...
    mov ecx, IA32_FMASK
    mov eax, 0x300          ; mask IF + TF
    mov edx, 0
    wrmsr
    ret

global fork_child_ret
fork_child_ret:
    leave
    mov rax, rbp
    ; rdx: distance between child and paarent stacks;
    mov rcx, 1<<62
    cmp rdx, rcx 
    jge .sub
    .add:
        add rax, rdx
        jmp .end_calc
    .sub:
        mov rcx, 1 << 63
        sub rcx, rdx
        sub rax, rcx
        jmp .end_calc
    .end_calc:
    mov rbp, rax
    xor rax, rax
    ret

global entry_syscall
extern syscall_dispatch
extern threads_info_ptr
entry_syscall:
    swapgs
    ; save current user rsp
    mov [gs:24], rsp
    ; user kernel rsp
    ; next time this run, rsp resetted to very top address of stack block, so no need to back it up
    mov rsp, [gs:16]
    ;
    push rbp
    push r11
    push r12
    push r13
    push r14
    push r15
    push rbx
    push rcx
    push rdi
    push rsi
    push rdx
    push r10
    push r8
    push r9
    push rax

    ;====================
    ;
    mov rdi, [rsp + 6*8]        ; syscall_number
    mov rsi, [rsp + 5*8] ;
    mov rdx, [rsp + 4*8] ; arg2 = rsi
    mov rcx, [rsp + 3*8] ; arg3 = rdx
    mov r8,  [rsp + 2*8] ; arg4 = r10
    mov r9,  [rsp + 1*8] ; arg5 = r8
    mov r10, rax ;

    call syscall_dispatch

    ;
    ;
    pop r9
    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    pop rcx
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop rbp
    ; restore user rsp
    mov rsp, [gs:24]
    mov rax, rax 
    ;ud2
    swapgs
    o64 sysret
    ;mov rbx, rbx

