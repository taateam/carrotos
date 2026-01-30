/* testapp.ld */
ENTRY(_start)
SECTIONS
{
  . = 0x400000; /* load address */
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(.bss*) }
}

