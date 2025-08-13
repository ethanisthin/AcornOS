__attribute__((section(".multiboot")))
const unsigned multiboot_header[] = {
    0x1BADB002, 
    0x00000003,  
    -(0x1BADB002 + 0x00000003) 
};

void _start() {
    volatile char *vga = (volatile char*)0xB8000;
    vga[0] = 'K'; 
    vga[1] = 0x1F;
    while(1);
}