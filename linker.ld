/* linker.ld */
ENTRY(start)

SECTIONS
{
    . = 0x6000;
    .boot : {
        KEEP(*(.multiboot_header))
    }
    . = 0x7200;
    stage2-low.text : {
        KEEP(*(core_start_stage2.text))
    }
    stage2-low.bss : {
        KEEP(*(stage2-low.bss))
        *(COMMON)
    }



    . = 1M;
    low.text : {
        *(low.text*)
    }
    low.data : {
        *(low.data*)
    }
    low.rodata : {
        *(low.rodata*)
    }
    low.bss (NOLOAD): {
        *(low.bss*)
        *(COMMON)
    }

    
    . = 0x400000000000 + 0x360000;
    .text : AT(0x360000) {
        *(.text*)
    }
    stage2-high.text :{
        . = ALIGN(0x1000);
        *(stage2-high.text*)
    }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data*)
    }
    stage2-high.data : {
        *(.data*)
    }
    
    seed.text : {
        KEEP(*(seed.text))
    }
    seed.data : {
        KEEP(*(seed.data))
    }
    seed.bss : {
        KEEP(*(seed.bss))
    }
    seed.rodata : {
        KEEP(*(seed.rodata))
    }
    seed.heap : {
        KEEP(*(seed.heap))
    }
    
    stage2-high.bss : {
        *(stage2-high.bss*)
        *(COMMON)
    }.bss : {
        *(.bss*)
        *(COMMON)
    } 
}
