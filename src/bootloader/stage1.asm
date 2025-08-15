org 0x7C00
bits 16

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
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl       
    push dx                    
    mov [0x7DFE], dx           

    mov si, msg_stage1
    call puts
    mov si, msg_drive
    call puts
    mov al, [boot_drive]
    call print_hex_byte
    mov si, newline
    call puts

    mov si, msg_loading
    call puts
    mov ah, 0x02      
    mov al, 4         
    mov ch, 0         
    mov dh, 0         
    mov cl, 2         
    mov dl, [boot_drive]
    mov bx, 0x07E0
    mov es, bx
    xor bx, bx
    int 0x13
    jc disk_error

    pop dx            
    jmp 0x07E0:0x0000

disk_error:
    mov si, msg_error
    call puts
    hlt

print_hex_byte:
    pusha
    mov bl, al
    shr al, 4
    call .nibble
    mov al, bl
    call .nibble
    popa
    ret
.nibble:
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jbe .print
    add al, 7
.print:
    mov ah, 0x0E
    int 0x10
    ret

boot_drive: db 0
msg_stage1:  db 'Stage1 started', 0x0D, 0x0A, 0
msg_drive:   db 'Boot drive: 0x', 0
msg_loading: db 'Loading Stage2...', 0x0D, 0x0A, 0
msg_error:   db 'Disk error!', 0x0D, 0x0A, 0
newline:     db 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xAA55