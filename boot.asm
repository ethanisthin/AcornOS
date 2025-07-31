[BITS 16]   
[ORG 0x7C00]

start:
    mov ax, 0x0003
    int 0x10
    mov si, hello_msg
    call print_string

    ; kernel loading
    mov si, loading_msg
    call print_string

    mov ah, 0x02
    mov al, 9 
    mov ch, 0
    mov cl, 2
    mov dh, 0
    ;mov dl, 0x80
    mov bx, 0x8000
    int 0x13

    jc disk_error

    mov si, success_msg
    call print_string

    cli  ;disable interrupts

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ;enter 32-bit mode
    jmp CODE_SEG:protected_mode_start

disk_error:
    mov si, error_msg
    call print_string
    jmp $

[BITS 32]
protected_mode_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x90000
    jmp 0x8000


[BITS 16]
print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    mov bh, 0x00
    int 0x10
    jmp print_string

done:
    ret

[BITS 32]
clear_screen:
    pusha
    mov edx, 0xB8000
    mov ecx, 80 * 25
    mov ax, 0x0F20

clear_loop:
    mov [edx], ax
    add edx, 2
    loop clear_loop
    popa
    ret

print_string_pm:
    pusha
    mov edx, 0xB8000

print_string_pm_loop:
    lodsb
    or al, al
    jz print_string_pm_done
    mov ah, 0x0F
    mov [edx], ax
    add edx, 2
    jmp print_string_pm_loop

print_string_pm_done:
    popa
    ret

; GDT stuff

gdt_start:
    dq 0
    
gdt_code:
    dw 0xFFFF 
    dw 0x0000 
    db 0x00
    db 10011011b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010011b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; segment selection constants
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start


hello_msg db 'AcornOS Lives!!!!!', 13, 10, 0
protected_msg db 'Protected Mode is active', 0
loading_msg db 'Loading kernel', 13,10,0
success_msg db 'Kernel loaded! Switching to protected mode', 13,10,0
error_msg db 'Disk error', 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55