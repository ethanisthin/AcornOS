org 0x7E00
bits 16

start:
    mov dx, [0x7C00 + 510]
    mov [boot_drive], dl
    
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    
    mov si, msg_stage2
    call puts
    
    call load_kernel
    
    mov si, msg_kernel_loaded
    call puts
    
    call enable_a20

    mov si, msg_entering_pmode
    call puts

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

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

load_kernel:
    mov si, msg_loading_kernel
    call puts
    
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]
    int 0x13
    jc .use_chs
    cmp bx, 0xAA55
    jne .use_chs
    test cl, 1
    jz .use_chs
    
    mov si, msg_using_lba
    call puts
    
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc .verify_load
    
.use_chs:
    mov si, msg_using_chs
    call puts
    
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    
    mov ah, 0x02
    mov al, 90 ;change here if needed
    mov ch, 0
    mov cl, 20
    mov dh, 0
    mov dl, [boot_drive]
    
    int 0x13
    jc .disk_error

.verify_load:
    mov ax, 0x1000
    mov es, ax
    mov ax, [es:0]
    cmp ax, 0
    je .disk_error
    ret
    
.disk_error:
    mov si, msg_disk_error
    call puts
    cli
    hlt

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

dap:
    db 0x10
    db 0
    dw 90 ;change here if needed
    dw 0x0000
    dw 0x1000
    dq 20

gdt_start:
    dd 0x00000000, 0x00000000    
    dd 0x0000FFFF, 0x00CF9A00    
    dd 0x0000FFFF, 0x00CF9200    
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

bits 32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    cld
    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, 11000 ;change here if needed
    rep movsd
    
    call 0x100000
    jmp $

bits 16
boot_drive: db 0

msg_stage2: db 'Stage2 started', 0x0D, 0x0A, 0
msg_loading_kernel: db 'Loading kernel...', 0x0D, 0x0A, 0
msg_using_lba: db 'Using LBA', 0x0D, 0x0A, 0
msg_using_chs: db 'Using CHS', 0x0D, 0x0A, 0
msg_kernel_loaded: db 'Kernel loaded', 0x0D, 0x0A, 0
msg_entering_pmode: db 'Entering protected mode', 0x0D, 0x0A, 0
msg_disk_error: db 'Disk error in stage2!', 0x0D, 0x0A, 0

times 2048-($-$$) db 0