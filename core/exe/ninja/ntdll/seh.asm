extern KiDispatchException
global KiTrapUser
KiTrapUser:
    /* RSP  = CPU trap frame */

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

    mov rdi, rsp       ; arg1 = trap_frame*
    sub rsp, 8        ; align stack
    call KiDispatchException
    add rsp, 8
;o