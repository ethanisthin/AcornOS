ENTRY(_start)

SECTIONS {
    . = 0x100000; /* 1MB mark */
    .multiboot : {
        *(.multiboot)
    }

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }
}