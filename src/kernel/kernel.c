#include "../drivers/vga.h"
#include "../arch/interrupts.h"
#include "../arch/pic.h"
#include "../drivers/keyboard.h"
#include "../shell/shell.h"
#include "../filesystem/fat16.h"
#include "../drivers/ata.h"


void kernel_main() {
    vga_init();    
    vga_printf("AcornOS - Shell System\n");
    vga_printf("======================\n\n");
    
    idt_init();
    pic_init();
    keyboard_init();
    
    vga_printf("Enabling interrupts...\n");
    __asm__ volatile ("sti");

    pic_enable_irq(1);    
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "System ready for shell!\n\n");

    ata_init();
    if (ata_identify()) {
        vga_printf("ATA drive detected and ready\n");
    } else {
        vga_printf_colored(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK,
                          "No ATA drive detected (using simulation mode)\n");
    }
    fat16_init();
    shell_init();
    shell_run();
}