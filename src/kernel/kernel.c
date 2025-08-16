#include "../drivers/vga.h"
#include "../arch/interrupts.h"
#include "../arch/pic.h"
#include "../drivers/keyboard.h"
#include "../shell/shell.h"

void kernel_main() {
    vga_init();
    
    vga_printf("AcornOS v0.1 - type 'help' for commands\n");
    vga_printf("======================\n\n");
    
    idt_init();
    pic_init();
    keyboard_init();
    
    vga_printf("Enabling interrupts...\n");
    __asm__ volatile ("sti");
    
    
    pic_enable_irq(1);  
    
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "System is Ready!\n\n");
    
    shell_init();
    shell_run();
}