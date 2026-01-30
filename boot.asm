section .multiboot_header
header_start:
    dd 0xe85250d6 ; multiboot2

    ;architecture
    dd 0 ; protected mode for i386

    ; header length
    dd header_end - header_start

    ; checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; env tag
    dw 0
    dw 0
    dd 8
header_end:

;================================================================

global start
global long_mode_start

section low.text
bits 32
start:
 
    cli
    mov esp, stack_top

    call copy_multiboot_info_to_0x10000
    call check_multiboot
    call check_cpuid
    call check_long_mode

    call setup_page_tables
    call get_cores_count
    call copy_core_start_bin_to_7000
    call start_all_aps
    call enable_lapic
    call setup_lapic_timer
    call disable_pic 
    call distribute_apic

    ; call enable_ps2_irqs
    call enable_paging

    ;mov eax, 0x12345678
    ;mov [0xFEE00030], eax
    ;mov eax, [0xFEE00030]
    ;mov [0xb8000], ax
    ;mov dword [0x00FF0000], 0xDEADBEEF
    ;mov eax, [0x00FF0000]


    ;call xxx
    lgdt [gdt64.pointer]
    jmp far [long_mode_target_a]

    hlt

copy_multiboot_info_to_0x10000:
    mov esi, ebx            ;
    mov edi, 0x10000        ;
    mov ecx, [esi]          ;
    rep movsb               ;
    ret
;============================================================

xxx:
    xor eax, eax
    ret
X32:
    mov edi, 0xB8000        ;
    mov eax, 0x0758         ;
    mov [edi], ax           ;
    ret
global print_hex
;================================================
check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret
.no_multiboot:
    mov al, "M"
    jmp error

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, "C"
    jmp error

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    
    ret
.no_long_mode:
    mov al, "L"
    jmp error

setup_page_tables:
    mov edi, page_table_l4     ;
    xor eax, eax               ;
    mov ecx, 512*4096/4            ; 513 blocks * 4KB = 2MB ⇒ 2MB / 4 = 524288 DWORDs
    rep stosd
    
    ;
    xor edx, edx
    mov eax, page_table_l3
    mov ebx, page_table_l4
.setup_l4_loop:
    mov ecx, eax
    or ecx, 0b11
    mov [ebx], ecx

    ;and ebx, 0x7FFFFFFF
    add eax, 4096
    add ebx, 8
    inc edx
    
    ;align 4096
    cmp edx, 128 
    jl .setup_l4_loop

    ; L3
    xor edi, edi                  ; low dword of physical address
    xor edx, edx                  ; edx = index
    mov eax, 0x0                  ; high dword of physical address
    mov ebx, page_table_l3        ;
.setup_l3_loop:
    ;
    or eax, 0x83
    mov [ebx], eax
    mov [ebx + 4], edi
    and eax, ~0x83                ;

    ; add 1GB: (edi:eax) += 0x40000000
    add eax, 0x40000000
    adc edi, 0

    ;and edi, 0x7FFFFFFF
    add ebx, 8                    ;
    inc edx
    cmp edx, 128*512              ;
    jl .setup_l3_loop

    ;
    xor edx, edx
    mov eax, page_table_l3
    mov ebx, page_table_l4_b
.setup_l4b_loop:
    mov ecx, eax
    or ecx, 0b11
    mov [ebx], ecx

    ;and ebx, 0x7FFFFFFF
    add eax, 4096
    add ebx, 8
    inc edx
    
    cmp edx, 128 
    jl .setup_l4b_loop

    ; L3
    xor edi, edi                  ; low dword of physical address
    xor edx, edx                  ; edx = index
    mov eax, 0x0                  ; high dword of physical address
    mov ebx, page_table_l3_b        ;
.setup_l3b_loop:
    ;
    or eax, 0x83
    mov [ebx], eax
    mov [ebx + 4], edi
    and eax, ~0x83                ;

    ; add 1GB: (edi:eax) += 0x40000000
    add eax, 0x40000000
    adc edi, 0

    ;and edi, 0x7FFFFFFF
    add ebx, 8                    ;
    inc edx
    cmp edx, 128*512              ;
    jl .setup_l3b_loop

    ret

enable_paging:
    ; pass page table location to cpu
    mov eax, page_table_l4
    mov cr3, eax

    ; enable PAE
    mov eax, cr4
    or eax, (1 << 5) | (1 << 7)  
    mov cr4, eax

    ; enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

error:
    ; print "ERROR: X", X representing the error code.
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov byte  [0xb800a], al
    hlt

;==========================================================================
%define LAPIC_BASE 0xFEE00000
%define ICR_LOW   0x300
%define ICR_HIGH  0x310
%define LAPIC_REG(offset)   dword [LAPIC_BASE + offset]
%define LAPIC_EOI           0xB0
%define LAPIC_SVR           0xF0
%define LAPIC_LVT_TIMER     0x320
%define LAPIC_TIMER_INIT    0x380
%define LAPIC_TIMER_CUR     0x390
%define LAPIC_TIMER_DIV     0x3E0
%define VECTOR_TIMER        0x31        ;

global get_cores_count
get_cores_count:
    mov eax, 1
    cpuid
    shr ebx, 16         ;
    and ebx, 0xFF       ;
    mov dword [cores_count], ebx
    ret

;
global get_current_apic_id
get_current_apic_id:
    mov eax, [LAPIC_BASE + 0x20] ; Local APIC ID register
    shr eax, 24
    ret

global copy_core_start_bin_to_7000
copy_core_start_bin_to_7000:
    mov edi, [multiboot_info_ptr]
    add edi, 8          ;

.find_tag:
    mov eax, [edi]      ; tag type
    cmp eax, 3          ; 3 = module
    jne .next_tag

    ;
    mov esi, [edi + 8]  ; mod_start
    mov ecx, [edi + 12] ; mod_end
    sub ecx, esi        ; size = end - start
    mov edi, 0x7000     ; destination addr
    rep movsb
    ret

.next_tag:
    mov eax, [edi + 4]  ; tag size
    add edi, eax
    add edi, 7
    and edi, 0xFFFFFFF8 ; align 8
    jmp .find_tag

global disable_pic
disable_pic:
    ;
    mov al, 0xFF
    out 0xA1, al   ; Slave PIC
    out 0x21, al   ; Master PIC
    ret

%define IOAPIC_BASE   0xFEC00000
%define IOREGSEL     IOAPIC_BASE
%define IOWIN        (IOAPIC_BASE + 0x10)

; eax = register index
; edx = value
ioapic_write:
    mov dword [IOREGSEL], eax
    mov dword [IOWIN], edx
    ret

enable_ps2_irqs:

    call ps2_wait_input_clear
    mov al, 0x20          ; Read Controller Command Byte
    out 0x64, al

    call ps2_wait_output_full
    in al, 0x60           ; AL = command byte

    or al, 0x03           ; bit0 = IRQ1, bit1 = IRQ12
                          ; (keyboard + mouse IRQ)
    and al, 0xDF          ; clear bit5 (disable translation)

    call ps2_wait_input_clear
    mov ah, al
    mov al, 0x60          ; Write Controller Command Byte
    out 0x64, al

    call ps2_wait_input_clear
    mov al, ah
    out 0x60, al

    ret

ps2_wait_input_clear:
    in al, 0x64
    test al, 2
    jnz ps2_wait_input_clear
    ret

ps2_wait_output_full:
    in al, 0x64
    test al, 1
    jz ps2_wait_output_full
    ret

distribute_apic:

    ; ===============================
    ; IRQ0 → vector 0x20 (timer)
    ; ===============================

    mov eax, 0x10            ; IRQ0 low
    mov edx, 0x00000020      ; vector 0x20, fixed, unmasked
    call ioapic_write

    mov eax, 0x11            ; IRQ0 high
    mov edx, 0x01000000      ; dest APIC ID = 1 (BSP)
    call ioapic_write

     ; ===============================
    ; IRQ1 → vector 0x21 (keyboard)
    ; ===============================

    mov eax, 0x12            ; 0x10 + 2*1
    mov edx, 0x00000021
    call ioapic_write

    mov eax, 0x13
    mov edx, 0x00000000     ; APIC ID = 0
    call ioapic_write

    ; ===============================
    ; IRQ12 → vector 0x2C (mouse)
    ; ===============================

    mov eax, 0x28            ; 0x10 + 2*12
    mov edx, 0x0000002C
    call ioapic_write

    mov eax, 0x29
    mov edx, 0x00000000
    call ioapic_write
    ret

global enable_lapic
enable_lapic:
    ; Enable xAPIC in MSR
    mov ecx, 0x1B
    rdmsr
    and eax, 0xFFFFF000 ; align base
    or eax, 1 << 11     ; set bit 11 (APIC global enable)
    or eax, 1 << 10     ; set xAPIC enable (bit 10, if supported)
    mov edx, 0
    wrmsr
    ret

global setup_lapic_timer
setup_lapic_timer:
    ; Enable APIC in SVR (Spurious Interrupt Vector Register)
    mov eax, 0x1FF            ; Vector = 0xFF, bit 8 = 1 (enable LAPIC)
    mov dword [0xFEE00000 + 0xF0], eax
    
    ; Set Divide Configuration = divide by 16 (value 0b0011)
    mov eax, 0x3
    mov dword [0xFEE00000 + 0x3E0], eax

    ; Set LVT Timer:
    ; vector = 0x31, mode = periodic (bit 17 = 1), unmasked (bit 16 = 0)
    mov eax, 0x20 | (1 << 17)
    mov dword [0xFEE00000 + 0x320], eax

    ;
    mov eax, 100000      ;
    mov dword [0xFEE00000 + 0x380], eax

    ret

%define SIPI_VECTOR 0x07   ; = 0x7000 >> 12

global start_all_aps
start_all_aps:
call get_current_apic_id
mov ebx, eax            ; ebx = current BSP apic id

mov ecx, 0              ;
.next_core:
    cmp ecx, [cores_count] ;  cores count
    jge .done

    ; send INIT IPI (assert)
    mov eax, ecx
    shl eax, 24                  ; APIC ID in high dword
    ;call xxx
    mov dword [LAPIC_BASE + ICR_HIGH], eax

    mov eax, 0x000C4500          ; INIT IPI (assert)
    mov dword [LAPIC_BASE + ICR_LOW], eax
    call short_delay

    ; send INIT IPI (deassert)
    mov eax, 0x000C0500          ; INIT IPI (deassert)
    mov dword [LAPIC_BASE + ICR_LOW], eax
    call short_delay

    ; send SIPI 1
    mov eax, 0x000C4600 | SIPI_VECTOR
    mov dword [LAPIC_BASE + ICR_LOW], eax
    call short_delay

    ; send SIPI 2
    mov eax, 0x000C4600 | SIPI_VECTOR
    call short_delay
    mov dword [LAPIC_BASE + ICR_LOW], eax


    inc ecx
    jmp .next_core
.done:
    ret
    
short_delay:
    ;
    mov ecx, 100000
    ret

multiboot_info_ptr: dd 0x10000

long_mode_target_a:
    dd long_mode_target_b
    dw 0x08 
bits 64
long_mode_target_b:
    ;mov rax, long_mode_start
    mov rax, testc
    jmp rax
print_hex:
    push rdi
    push rcx
    push rax

    mov rdi, 0xb8000        ; VGA text buffer
    mov rcx, 16             ; 16 hex digits
    mov rax, rax            ;

.print_loop:
    rol rax, 4              ;
    mov bl, al
    and bl, 0x0F            ;
    cmp bl, 9
    jbe .num
    add bl, 'A' - 10
    jmp .print
.num:
    add bl, '0'
.print:
    mov [rdi], bl
    add rdi, 2              ;
    loop .print_loop

    pop rax
    pop rcx
    pop rdi
    ret
;
global testt
testt: dq 25;0x6868686868686868
dq 256
testc:
    ;call X32
    mov rbx, testt;
    mov rcx, 64*1024* 0x40000000 ;0x800000000000
    add rbx, rcx
    mov rax, [rbx]
    
    ;mov rbx, page_table_l3
    ;add rbx, 8*(1024)
    ;mov rax, [rbx]
    
    ;mov rbx, page_table_l4
    ;add rbx, 1*8
    ;mov rcx, [rbx]
    ;and rcx, 0x000FFFFFFFFFF000
    ;mov rax, rcx;[rcx]
    
    ;call print_hex
    mov rax, long_mode_start
    jmp rax 
    jmp $
    ret
bits 32
section low.data
global cores_count
    cores_count: dq 2
section low.rodata
global gdt64
gdt64:
    dq 0 ; zero entry
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.data_segment: equ $ - gdt64
    dq (1 << 44) | (1 << 47) | (1 << 41)          ; RW data segment (kernel)
.user_data_segment: equ $ - gdt64
    dq (1 << 44) | (1 << 47) | (3 << 45) | (1 << 41) ; data, DPL=3
.user_code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (3 << 45) | (1 << 53) ; code, DPL=3
.tss_segment: equ $ - gdt64               ;
    dw 103         ; Limit (will patch)   ; 2 byte
    dw 0           ; tss addr 0:15            ; 2 byte
    db 0           ; tss addr 16:23           ; 1 byte
    db 0x89        ; Type 0x9 (TSS64 avail) + P=1  ; 1 byte
    db 0           ; Limit high (ignored) ; 1 byte
    db 0           ; tss addr 24:31           ; 1 byte
    dd 0           ; tss addr 32:63           ; 4 byte
    dd 0           ; Reserved             ; 4 byte
.pointer:
    dw $ - gdt64 - 1 ; length
    dq gdt64 ; address

section low.bss nobits
alignb 4096
global page_table_l4
page_table_l4:
    ;resb 2048
    resb 1024
global page_table_l4_b
page_table_l4_b:
    ;resb 2048
    resb 1024
alignb 4096
global page_table_l3
page_table_l3:
    resb 4096 * 256
global page_table_l3_b
page_table_l3_b:
    resb 4096 * 256
alignb 64
stack_bottom:
    resb 4096
stack_top:


;=============================================================
%assign MAX_CORES_COUNTv 256
%assign MAX_THREADS_COUNTv 2560
%assign MAX_PROCESSES_COUNTv 2560 / 2
%assign KERNEL_STACK_SIZEv 4 * 4096
%assign MAX_FDSv 10240
%assign MAX_PIPESv 10240
section .rodata

global MAX_CORES_COUNT
    MAX_CORES_COUNT: dq MAX_CORES_COUNTv

global MAX_THREADS_COUNT
    MAX_THREADS_COUNT: dq MAX_THREADS_COUNTv

global MAX_PROCESSES_COUNT
    MAX_PROCESSES_COUNT: dq MAX_PROCESSES_COUNTv
    
global KERNEL_STACK_SIZE
    KERNEL_STACK_SIZE: dq KERNEL_STACK_SIZEv

global current_tick
    current_tick: dq 0

;=============================================================

section .text
bits 64
setup_tss_core0:
    ; register high addr gdt64
    mov rbx, gdt64.pointer
    add rbx, 2
    mov rcx, gdt64
    mov rax, 0x400000000000 
    add rcx, rax
    mov [rbx], rcx
    lgdt [gdt64.pointer]
    ; rsp0 -> tss
    mov rax, rsp0_core0_stack_top
    ;
    mov rbx, tss64_core0
    add rbx, 4
    mov [rbx], rax   ; RSP0 in TSS
    ;   tss -> gdt
    mov     rax, tss64_core0           ;
    mov     word [gdt64 + gdt64.tss_segment], 103
    mov     word [gdt64 + gdt64.tss_segment + 2], ax          ; base 0:15
    shr     rax, 16
    mov     byte [gdt64 + gdt64.tss_segment + 4], al          ; base 16:23
    mov     byte [gdt64 + gdt64.tss_segment + 5], 0x89        ; Type+S+DPL+P
    mov     byte [gdt64 + gdt64.tss_segment + 6], 0x0         ; high limit
    shr     rax, 8
    mov     byte [gdt64 + gdt64.tss_segment + 7], al          ; base 24:31
    shr     rax, 8
    mov     dword [gdt64 + gdt64.tss_segment + 8], eax        ; base 32:63
    mov     dword [gdt64 + gdt64.tss_segment + 12], 0x40      ; reserved 0x40
    ; reload tr register
    ;
    mov ax, gdt64.tss_segment      ;
    ltr ax                         ;
    ret
global X64
X64:
    mov rbx, 0x400000000000
    mov rdi, 0xB8000        ;
    add rdi, rbx
    mov rax, 0x0758         ;
    mov [rdi], ax           ;
    ret
global remap_pic
remap_pic:
    ; remap master PIC to 0x20–0x27
    mov al, 0x11
    out 0x20, al       ; init master
    out 0xA0, al       ; init slave

    mov al, 0x20       ; vector offset for master PIC (32)
    out 0x21, al
    mov al, 0x28       ; vector offset for slave PIC (40)
    out 0xA1, al

    mov al, 0x04       ; tell master PIC about slave at IRQ2
    out 0x21, al
    mov al, 0x02       ; tell slave PIC its cascade identity
    out 0xA1, al

    mov al, 0x01
    out 0x21, al
    out 0xA1, al
    ;lllllllll
    mov al, 0b11111001      ; = 0xF9 = mask all IRQs except bit 1, 2 (IRQ1, 2)
    out 0x21, al            ;
    mov al, 0b11101111      ; 0xFF = mask all IRQs except bit 1, 2 (IRQ1, 2)
    out 0xA1, al            ;
    ; IRQ 0 → vector 0x20
    mov dword [0xFEC00000 + 0x10], eax
    mov dword [0xFEC00000 + 0x11], eax

    ret

enable_mouse:
    ; enable aux port
    call wait_ps2_write
    mov al, 0xA8
    out 0x64, al

    ; read command byte
    call wait_ps2_write
    mov al, 0x20
    out 0x64, al
    call wait_ps2_read
    in  al, 0x60

    ; enable IRQ12
    or  al, 0x02
    mov bl, al

    ; write command byte back
    call wait_ps2_write
    mov al, 0x60
    out 0x64, al
    call wait_ps2_write
    mov al, bl
    out 0x60, al

    ; tell controller next byte is for mouse
    call wait_ps2_write
    mov al, 0xD4
    out 0x64, al

    ; enable data reporting
    call wait_ps2_write
    mov al, 0xF4
    out 0x60, al

    ; read ACK (0xFA)
    call wait_ps2_read
    in al, 0x60

    ret

; Wait for PS/2 controller input buffer to be empty
wait_ps2_write:
    in al, 0x64
    test al, 0x02          ; Test bit 1 (input buffer status)
    jnz wait_ps2_write
    ret

; Wait for PS/2 controller output buffer to have data
wait_ps2_read:
    in al, 0x64
    test al, 0x01          ; Test bit 0 (output buffer status)
    jz wait_ps2_read
    ret
    
global long_mode_start
long_mode_start:
    ;mov rax, 0x1000 
    ;sti
    mov rax, stack64_top_core0
    mov rsp, rax
    mov rbp, rsp

    ; load null into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; call remap_pic
    call enable_mouse;
    extern get_total_memory
    call get_total_memory
    extern mark_reserved_blocks_as_used
    call mark_reserved_blocks_as_used
    extern register_interrupt_handlers
    call register_interrupt_handlers
    mov rax, 0xFEE000F0
    mov rbx, [rax]             ; set bit 8 = enable
    ;extern enable_lapic
    ;call enable_lapic
    ;extern init_pit
    ;call init_pit ; timer tick
    ;extern disable_pic
    ;call disable_pic
    extern setup_syscall
    call setup_syscall
    extern setup_tss_core0 
    call setup_tss_core0 
    ;=================
    ;=================
    extern kernel_main
    call kernel_main
    hlt

global halt
halt:
    jmp $

global change_cr3
change_cr3:
    mov cr3, rdi
    ret
;============================================
section .data
;============================================\

align 8
global start_flag
    start_flag: dq 0
global core0_prepared
    core0_prepared: dq 0
global total_memory
    total_memory: dq 2
align 8
global total_memory_blocks_count
    total_memory_blocks_count: dq 0
align 8
global multiboot_info_ptr_64
    multiboot_info_ptr_64: dq 0x10000

alignb 8
global input_buffer_w_index
input_buffer_w_index:
    dq 0
alignb 8
global input_buffer_r_index
input_buffer_r_index:
    dq 0
;===========================================
section .bss
;===========================================

; ====== core, idt, gdt ==================
alignb 4096
stack64_bottom_core0: ; stack for main core
    resb 4*4096 
stack64_top_core0:
    ;resb 32 

xsleep_bottom:
    resb 4096
global xsleep_top
xsleep_top:

resb 32
alignb 4096
global rsp0_core0_stack_bottom
rsp0_core0_stack_bottom: ; stack for main core
    resb 2*4096
rsp0_core0_stack_top:

alignb 16
global tss64_core0
tss64_core0:
    resb 104

alignb 16
global idt_table
idt_table:
    resb 16 * 256      ;
alignb 16
global idt_ptr
idt_ptr:
    resw 1             ; limit
    resq 1             ; base
alignb 16
global cores_info
cores_info:
    resb 5*8 * MAX_CORES_COUNTv   ;
; =========== processes & threads ===========
alignb 16
global processes_info_ptr
processes_info_ptr:
    resb 8 * MAX_PROCESSES_COUNTv
alignb 16
global processes_info_lock
    processes_info_lock: resq 1
alignb 16
global processes_cr3_ptr
processes_cr3_ptr:
    resb 8 * MAX_PROCESSES_COUNTv
alignb 16
global threads_info_ptr
threads_info_ptr:
    resb 8 * MAX_THREADS_COUNTv
global threads_info_lock
    threads_info_lock: resq 1
; ============== inputs & terminal ==============
alignb 64
global mouse_buffer
mouse_buffer:
    resb 4096
alignb 64
global input_buffer
input_buffer:
    resb 4096
alignb 64
global tty_global
tty_global:
    resb 8*512
alignb 16
global fd_global
fd_global:
    resb 8 * MAX_FDSv
alignb 16
global pipe_global
pipe_global:
    resb 8 * MAX_PIPESv
; ======= networks ==================
alignb 16
global nic_global
nic_global: 
    resb 4096
alignb 16
global listen_global
listen_global: 
    resb 40960
alignb 16
global uds_listen_global
uds_listen_global: 
    resb 40960
alignb 16
global socket_global
socket_global: 
    resb 40960
alignb 16
global uds_socket_global
uds_socket_global: 
    resb 40960
alignb 16
global mac_cache_v4
    mac_cache_v4: resb 40960
alignb 16
global route_table_v4
    route_table_v4: resb 40960
alignb 16
global fb_global
    fb_global: resb 1024
alignb 16
global sm_global
    sm_global: resb 4096
;=== disk & driver =========================
alignb 16
global partition_global
    partition_global: resb 4096 * 2
alignb 16
global partition_driver_global
    partition_driver_global: resb 4096 * 4
alignb 16
global mount_global
    mount_global: resb 4096
alignb 16
global driver_global
    driver_global: resb 4096 * 2
alignb 16
global device_global
    device_global: resb 4096 * 4
;======= users =================================
alignb 16
global user_global
    user_global: resb 4096 
;======= memory map =========================
alignb 16
global memory_map_lock
    memory_map_lock: resq 1
alignb 8
global end_kernel
end_kernel:
global memory_map
memory_map:
    resb 1