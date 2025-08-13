org 0x7C00
bits 16

%define ENDL 0x0D, 0x0A

start:
    jmp main

puts:
    push si
    push ax
    push bx

.loop:
    lodsb               
    or al, al           
    jz .done

    mov ah, 0x0E        
    mov bh, 0           
    int 0x10

    jmp .loop

.done:
    pop bx
    pop ax
    pop si    
    ret
    

main:
    mov ax, 0           
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    mov si, msg_hello
    call puts
    cli
    hlt

.halt:
    jmp .halt

boot_drive: db 0
msg_hello: db 'AcornOS Lives Again!!!! (from a hard disk)', ENDL, 0

times 446-($-$$) db 0

db 0x80
times 15 db 0
times 16*3 db 0
dw 0AA55h