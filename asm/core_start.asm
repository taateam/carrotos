[BITS 16]
[ORG 0x7000]

start:
    cli

    ;hlt
    ;jmp $
    ;
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00         ;

    ;
   
    ;lidt [idt_descriptor]
    lgdt [gdt_descriptor]
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    ;jmp X32
    jmp 0x08:0x7200
    
; ========== GDT & descriptor ==========
gdt_start:
    dq 0x0000000000000000         ; null
    dq 0x00CF9A000000FFFF         ; code segment
    dq 0x00CF92000000FFFF         ; data segment
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

align 16
;global idt_table
;idt_table:
;

;idt_descriptor:
;
;

X:
    mov ax, 0xB800  ; segment video memory text mode
    mov es, ax
    mov di, 0        ;
    mov al, 'X'      ;
    mov ah, 0x07     ;
    mov [es:di], ax  ;
    jmp $

