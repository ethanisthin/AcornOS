ENTRY(kernel_main)

SECTIONS {
    . = 0x100000;

    _kernel_start = .;

    .text : {
        *(.text)
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    . = ALIGN(4096);

    .bss : {
        *(COMMON)
        *(.bss)
    }

    _kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}