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


detect_memory:
    mov di, 0x5004      
    xor ebx, ebx        
    mov edx, 0x534D4150 
    mov dword [0x5000], 0 

.mem_loop:
    mov eax, 0xE820
    mov ecx, 24         
    int 0x15
    jc .mem_error       

    inc dword [0x5000]  
    add di, 24          

    test ebx, ebx       
    jnz .mem_loop

.mem_error:
    ret                 

main:
    cli
    mov ax, 0           
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, msg_hello
    call puts

    call enable_a20

    call detect_memory

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp CODE_SEG:start_protected_mode

enable_a20:
    in al, 0x92
    test al, 2
    jnz .a20_done      
    or al, 2
    and al, 0xFE       
    out 0x92, al
.a20_done:
    ret

[bits 32]
start_protected_mode:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x90000

    mov ecx, [0x5000]  
    test ecx, ecx
    jz .no_memory

    jmp $

.no_memory:
    mov word [0xB8000], 0x4F4D
    jmp $

boot_drive: db 0
msg_hello: db 'AcornOS: Entering Protected Mode...', ENDL, 0

gdt_start:
    dq 0x0             

gdt_code:              
    dw 0xFFFF          
    dw 0x0             
    db 0x0             
    db 10011010b       
    db 11001111b       
    db 0x0             

gdt_data:              
    dw 0xFFFF          
    dw 0x0             
    db 0x0             
    db 10010010b       
    db 11001111b       
    db 0x0             

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 446-($-$$) db 0

db 0x80                
db 0x00, 0x01, 0x00    
db 0x06                
db 0x00, 0x01, 0x00    
dd 0x00000001          
dd 0x00000020          

times 16*3 db 0

dw 0xAA55