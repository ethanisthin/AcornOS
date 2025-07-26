[BITS 16]   
[ORG 0x7C00]

start:
    ;
    mov ax, 0x0003
    int 0x10
    mov si, hello_msg
    call print_string

    jmp $

print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string

done:
    ret

hello_msg db 'AcornOS Lives!!!!!'

times 510-($-$$) db 0

dw 0xAA55