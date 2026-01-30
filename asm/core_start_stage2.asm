[BITS 32]
[GLOBAL protected_mode]

%define STACK_SIZE     512     ; 512 B
%define MAX_CORES      256

extern page_table_l4

section stage2-low.text
protected_mode:
    ;
    cli
    mov ax, 0x10       ; data segment selector trong GDT (entry 2)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax         ;
    ;
    mov eax, 1          ; CPUID leaf 1
    cpuid
    shr ebx, 24         ; EBX[31:24] = APIC ID
    mov eax, ebx        ;
    
    inc eax                   ;
    mov edx, STACK_SIZE
    mul edx                   ; eax = offset stack
    add eax, stack32_bottom_aps
    mov esp, eax
    ;call X32
    ;mov eax, long_mode_target1
    ;call print_hex

    ;lapic
    extern enable_lapic
    call enable_lapic
    extern setup_lapic_timer
    call setup_lapic_timer
    ;sti
    ;extern disable_pic
    ;call disable_pic 
    
    ; Load GDT
    lgdt [gdt64a.pointer]

    ; Set LME (long mode) trong EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ;enable pae
    mov eax, cr4
    or eax, 1 << 5     ;
    mov cr4, eax
    
    ;
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable paging
    ;call X32
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ;
    jmp far [long_mode_target_a1]

X32:
    mov edi, 0xB8000        ;
    mov eax, 0x0758         ;
    mov [edi], ax           ;
    ret

print_hex1:
    pushad
    mov edi, 0xb8000        ; VGA text buffer
    mov ecx, 8              ; 8 hex digits
    mov ebx, eax            ;

.next_digit:
    mov eax, ebx
    shl eax, 4              ;
    shr eax, 32             ;
    and al, 0x0F            ;

    cmp al, 10
    jl .is_digit
    add al, 'A' - 10
    jmp .store
.is_digit:
    add al, '0'

.store:
    mov ah, 0x0F            ;
    mov [edi], ax           ;
    add edi, 2
    shl ebx, 4              ;
    loop .next_digit

    popad
    ret
    
long_mode_target_a1:
    dd long_mode_target_b1
    dw 0x08
[BITS 64]
extern long_mode_start1
long_mode_target_b1:
    ;mov rax, long_mode_start1
    mov rax, testc_aps
    jmp rax
testc_aps:
    ;jmp $
    ;mov rax, [0x200000100000]
    mov rax, long_mode_start1 
    jmp rax
    ;call X32
    ;ret
[BITS 32]
; ----------------------
; GDT cho Long Mode
; ----------------------
align 16
gdt64a:
    dq 0 ; zero entry
.code_segment: equ $ - gdt64a
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.pointer:
    dw $ - gdt64a - 1 ; length
    dq gdt64a ; address


%assign MAX_CORES_COUNTv 256

section stage2-low.bss nobits
stack32_bottom_aps:
    resb STACK_SIZE 
stack32_top_aps:
    resb 4096

[BITS 64]
%define STACK_SIZE1     (4096*4)     ;
extern gdt64a_all
extern read_apic_id
section .text

setup_tss_core_aps:
    call read_apic_id
    mov rsi, rax ; rsi: core_id
    mov rbx, rsi
    imul rbx, 80; 56
    mov rcx, gdt64_aps
    add rbx, rcx
    mov rdi, rbx    ; rdi: gdt64 of this core
    mov r8, rdi
    add r8, 8       ; r8: kernel code 0x8
    mov r9, rdi
    add r9, 16      ; r9: kernel data 
    mov r10, rdi
    add r10, 24     ; r10: user data 
    mov r11, rdi
    add r11, 32     ; r11: user code 
    mov r12, rdi
    add r12, 40     ; r12: tss
    mov r13, rdi
    add r13, 56     ; r13: .pointer
    ; register high addr gdt64
    ; 0
    mov qword [rdi], 0
    ; kernel code 0x8
    mov rax, (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
    mov [r8], rax 
    ; kernel data
    mov rax, (1 << 44) | (1 << 47) | (1 << 41)
    mov [r9], rax 
     ; user data
    mov rax, (1 << 44) | (1 << 47) | (3 << 45) | (1 << 41)
    mov [r10], rax 
    ; user code 
    mov rax, (1 << 43) | (1 << 44) | (1 << 47) | (3 << 45) | (1 << 53)
    mov [r11], rax 
   ; tss
    ; gdt -> tss 
    mov     r14, tss64_core_aps
    mov     rbx, rsi
    imul    rbx, 128
    add     r14, rbx; r14: tss_addr of this core
    mov     rax, r14
    mov     word [r12], 103
    mov     word [r12 + 2], ax          ; base 0:15
    shr     rax, 16
    mov     byte [r12 + 4], al          ; base 16:23
    mov     byte [r12 + 5], 0x89        ; Type+S+DPL+P
    mov     byte [r12 + 6], 0x0         ; high limit
    shr     rax, 8
    mov     byte [r12 + 7], al          ; base 24:31
    shr     rax, 8
    mov     dword [r12 + 8], eax        ; base 32:63
    mov     dword [r12 + 12], 0x40      ;
    ; gdt64_aps.pointer
    mov rbx, r13 ;gdt64_ap.pointer
    mov word [rbx], 55
    add rbx, 2
    mov rcx, rdi ;gdt64_ap
    ;mov rax, 0x400000000000 
    ;add rcx, rax
    mov [rbx], rcx
    ;tss -> rsp0
    mov r15, rsp0_core_aps_stack_top
    mov rax, rsi
    ;add rax, 1
    imul rax, 4096
    sub r15, rax; r15: tss_stack_top_of this core
    ;mov rcx, 0x400000000000
    ;add rbx, rcx; rbx : top addr of rsp0 of this core
    mov rax, r14
    add rax, 4 ; rax: 4th byte of tss 104 bytes of this core
    mov [rax], r15   ; RSP0 in TSS
    ; reload tr register
    ;
    lgdt [r13]
    mov rbx, r12
    sub rbx, rdi
    mov rax, 0
    mov ax, bx      ;
    ltr ax                         ;
    ;ud2
    ret

long_mode_start1:
    ;sti
    mov ecx, 0x1B
    rdmsr                      ;
    mov eax, 0xFEE00000
    mov dword [eax], 1
    mov eax, esp    ;
    mov rsp, rax    ;
    mov rbp, rsp
    
    ; Clear segment registers
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call read_apic_id
    
    ;
    mov rcx, rax
    mov rax, rcx
    inc rax                   ;
    mov rdx, STACK_SIZE1
    mul rdx                   ; eax = offset stack
    mov rdi, stack64_bottom_aps
    add rax, rdi
    mov rsp, rax
    
    ;======================================
    extern remap_pic
    call remap_pic
    ;extern register_interrupt_handlers
    ;call register_interrupt_handlers
    ;extern init_pit
    ;call init_pit ; timer tick
    extern setup_syscall
    call setup_syscall
    extern setup_tss_core_aps;setup_tss_core0 
    call setup_tss_core_aps;setup_tss_core0 
    ; wait
    extern core0_prepared
wait_core0_prepared:
    mov rax, core0_prepared
    mov rax, [rax]
    cmp rax, 0
    je wait_core0_prepared
    ; done wait
    ;lidt
    extern idt_ptr
    mov rbx, idt_ptr
    lidt [rbx]
    ;sti
    ;=======================================
    extern core_main
    call core_main
halt:
    hlt
    jmp halt


; ----------------------
;
; ----------------------

section stage2-high.data
align 4

section stage2-high.bss nobits

align 4096
global rsp0_core_aps_stack_bottom
rsp0_core_a_stack_bottom: ; stack for all core
    resb MAX_CORES_COUNTv*4096
rsp0_core_aps_stack_top:

align 16
gdt64_aps:
    resb MAX_CORES_COUNTv * 80

align 16
global tss64_core_aps
tss64_core_aps:
    resb MAX_CORES_COUNTv*128

align 16
global stack64_bottom_aps
stack64_bottom_aps:
    resb STACK_SIZE1 * MAX_CORES
stack64_top_aps:
