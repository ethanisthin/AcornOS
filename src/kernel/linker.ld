ENTRY(kernel_main)

SECTIONS {
    . = 0x100000;

    .text : {
        *(.text)
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}