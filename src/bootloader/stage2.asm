org 0x7E00
bits 16

start:
    mov dx, [0x7C00 + 510]
    
    mov ah, 0x0E
    mov al, 'D'
    int 0x10
    mov al, ':'
    int 0x10
    mov al, dl  
    call print_hex_byte
    
    cli
    hlt

print_hex_byte:
    mov ah, 0x0E
    push ax
    shr al, 4
    call .nibble
    pop ax
    and al, 0x0F
.nibble:
    add al, '0'
    cmp al, '9'
    jbe .print
    add al, 7
.print:
    int 0x10
    ret

times 512 db 0  