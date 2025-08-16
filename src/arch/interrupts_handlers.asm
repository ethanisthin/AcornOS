section .text

; Macro to create interrupt handlers without error codes
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli                 ; Disable interrupts
    push byte 0         ; Push dummy error code
    push byte %1        ; Push interrupt number
    jmp isr_common_stub ; Jump to common handler
%endmacro

; Macro to create interrupt handlers with error codes
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli                 ; Disable interrupts
    push byte %1        ; Push interrupt number
    jmp isr_common_stub ; Jump to common handler
%endmacro

; Macro to create IRQ handlers
%macro IRQ 2
global irq%1
irq%1:
    cli                 ; Disable interrupts
    push byte 0         ; Push dummy error code
    push byte %2        ; Push interrupt number
    jmp irq_common_stub ; Jump to IRQ handler
%endmacro

; Create interrupt service routines for exceptions (0-31)
ISR_NOERRCODE 0     ; Division by zero
ISR_NOERRCODE 1     ; Debug
ISR_NOERRCODE 2     ; Non-maskable interrupt
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; Bound range exceeded
ISR_NOERRCODE 6     ; Invalid opcode
ISR_NOERRCODE 7     ; Device not available
ISR_ERRCODE   8     ; Double fault
ISR_NOERRCODE 9     ; Coprocessor segment overrun
ISR_ERRCODE   10    ; Invalid TSS
ISR_ERRCODE   11    ; Segment not present
ISR_ERRCODE   12    ; Stack-segment fault
ISR_ERRCODE   13    ; General protection fault
ISR_ERRCODE   14    ; Page fault
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; x87 floating-point exception
ISR_ERRCODE   17    ; Alignment check
ISR_NOERRCODE 18    ; Machine check
ISR_NOERRCODE 19    ; SIMD floating-point exception
ISR_NOERRCODE 20    ; Virtualization exception
ISR_NOERRCODE 21    ; Reserved
ISR_NOERRCODE 22    ; Reserved
ISR_NOERRCODE 23    ; Reserved
ISR_NOERRCODE 24    ; Reserved
ISR_NOERRCODE 25    ; Reserved
ISR_NOERRCODE 26    ; Reserved
ISR_NOERRCODE 27    ; Reserved
ISR_NOERRCODE 28    ; Reserved
ISR_NOERRCODE 29    ; Reserved
ISR_ERRCODE   30    ; Security exception
ISR_NOERRCODE 31    ; Reserved

; Create IRQ handlers (32-47)
IRQ 0, 32    ; Timer
IRQ 1, 33    ; Keyboard
IRQ 2, 34    ; Cascade (never raised)
IRQ 3, 35    ; COM2
IRQ 4, 36    ; COM1
IRQ 5, 37    ; LPT2
IRQ 6, 38    ; Floppy disk
IRQ 7, 39    ; LPT1
IRQ 8, 40    ; CMOS real-time clock
IRQ 9, 41    ; Free for peripherals
IRQ 10, 42   ; Free for peripherals
IRQ 11, 43   ; Free for peripherals
IRQ 12, 44   ; PS2 mouse
IRQ 13, 45   ; FPU / Coprocessor / Inter-processor
IRQ 14, 46   ; Primary ATA hard disk
IRQ 15, 47   ; Secondary ATA hard disk

; External functions to handle interrupts (defined in C)
extern interrupt_handler
extern irq_handler

; Common interrupt handler stub for exceptions
isr_common_stub:
    ; Save all registers
    pusha
    
    ; Save data segment descriptor
    mov ax, ds
    push eax
    
    ; Load kernel data segment descriptor
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C interrupt handler
    call interrupt_handler
    
    ; Restore data segment descriptor
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Restore all registers
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    sti
    iret

; Common IRQ handler stub
irq_common_stub:
    ; Save all registers
    pusha
    
    ; Save data segment descriptor
    mov ax, ds
    push eax
    
    ; Load kernel data segment descriptor
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C IRQ handler
    call irq_handler
    
    ; Restore data segment descriptor
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Restore all registers
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret


; Export interrupt handler addresses for C code
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
global isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

; Export IRQ handler addresses for C code
global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
global irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15