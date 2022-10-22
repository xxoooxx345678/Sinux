SECTIONS
{
    . = 0x80000 - 8192;
    _bootloader_start = .;
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    _data = .;
    .data : { *(.data) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _end = .;
}
__loader_size = (_end - _bootloader_start)>>3;