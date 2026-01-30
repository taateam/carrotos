section .text

global isr_keyboard_wrapper
extern isr_keyboard_handler  ;
isr_keyboard_wrapper:
    cli
    ;
    call swap_gs_if_from_user_mode
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    in al, 0x64
    test al, 0x20          ; bit 5 = mouse?
    jnz .done               ;
        xor rax, rax
        in al, 0x60            ;
        mov rdi, rax
        ;
        call isr_keyboard_handler

    .done:
    ;
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
    pop rbp
    pop rbx
    pop rdx
    ;pop rax

    ;mov al, 0x20
    ;
    xor rcx, rcx
    mov ecx, 0xFEE00000 + 0xB0
    mov rax, 0x400000000000
    add rcx, rax
    xor rax, rax
    in al, 0x60
    mov dword [rcx], 0
    pop rcx
    pop rax
    call swap_gs_if_from_user_mode
    iretq
    
global isr_mouse_wrapper
extern isr_mouse_handler  ;
isr_mouse_wrapper:
    cli
    ;
    call swap_gs_if_from_user_mode
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    in al, 0x64
    test al, 0x20          ; bit 5 = mouse?
    jz .done               ;
        xor rax, rax
        in al, 0x60            ;
        mov rdi, rax
        ;
        call isr_mouse_handler
    .done:
    ;
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
    pop rbp
    pop rbx
    pop rdx
    ;pop rax

    ;mov al, 0x20
    ;out 0xA0, al        ; EOI to slave PIC
    ;
    xor rcx, rcx
    mov ecx, 0xFEE00000 + 0xB0
    mov rax, 0x400000000000
    add rcx, rax
    xor rax, rax
    in al, 0x60
    mov dword [rcx], 0
    pop rcx
    pop rax
    call swap_gs_if_from_user_mode
    iretq
    
global isr_e1000_wrapper
extern nic0_isr
isr_e1000_wrapper:
    cli
    ;
    call swap_gs_if_from_user_mode
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ;
    call nic0_isr

    ;
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
    pop rbp
    pop rbx
    pop rdx
    
    mov rax, 0x400000000000
    mov rcx, 0xFEE000B0; lapic mmio
    add rax, rcx
    mov dword [rax], 0;
    pop rcx
    pop rax
    call swap_gs_if_from_user_mode
    iretq
    
global sse_mode
sse_mode: db 0

global invalid_opcode_handler
invalid_opcode_handler:
    cli
    ;
    call swap_gs_if_from_user_mode
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    extern switch_context_type
    call switch_context_type
    ;
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
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    call swap_gs_if_from_user_mode
    iretq
    
global seg_fault_handler
seg_fault_handler:
    cli
    ;
    call swap_gs_if_from_user_mode
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    extern seg_fault_signal 
    call seg_fault_signal  
    ;
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
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    call swap_gs_if_from_user_mode
    iretq
    
global float_error_handler
float_error_handler:
    cli
    call swap_gs_if_from_user_mode
    ;
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    extern seg_fault_signal 
    call seg_fault_signal  
    ;
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
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    call swap_gs_if_from_user_mode
    iretq
    
global swap_gs_if_from_user_mode
swap_gs_if_from_user_mode:
    push rax
    mov rax, rsp
    add rax, 24
    mov rax, [rax]
    and rax, 3
    jz .from_kernel
        swapgs
    .from_kernel:
    pop rax
    ret

global gsbase_restore
gsbase_restore:
    ;swapgs
    mov rax, rdi;
    and rax, 0xFFFFFFFF;
    mov rdx, rdi;
    shr rdx, 32;
    mov ecx, 0xC0000102;
    wrmsr;
    ;swapgs
    ret;r

global xsave_context
xsave_context:
    push rax
    push rdx

    mov eax, 0x7          ; x87 + SSE + YMM
    mov edx, 0x0
    xsave [rdi]

    pop rdx
    pop rax
    ret

; rdi = pointer to xsave_area
global xload_context
xload_context:
    push rax
    push rdx

    mov eax, 0x7
    mov edx, 0x0
    xrstor [rdi]

    pop rdx
    pop rax
    ret
;:
;q
