#include "kernel.h"
#include "vga.h"
#include "interrupts.h"
#include "io.h"
#include "kbm.h"
#include "shell.h"

void _start() {
    __asm__ volatile("cli");
    __asm__ volatile("mov $0x90000, %esp");
    
    set_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLACK);
    clr_scr();
    idt_install();
    
    pic_remapper(0x20, 0x28);
    outb(PIC1_DATA, inb(PIC1_DATA) & ~0x02);  
    
    while (inb(KBD_STATUS_PORT) & 0x02);     
    outb(KBD_DATA_PORT, 0xF4);              

    irq_handle_install(1, kbm_handler);
    
    __asm__ volatile("sti");
    
    shell_init();
    
    while(1) {
        __asm__ volatile("hlt");
    }
}