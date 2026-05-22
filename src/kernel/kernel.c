#include "../drivers/vga.h"
#include "../arch/interrupts.h"
#include "../arch/pic.h"
#include "../drivers/keyboard.h"
#include "../shell/shell.h"
#include "../filesystem/fat16.h"
#include "../drivers/ata.h"
#include "../debug_params.h"
#include "../arch/timer.h"


void kernel_main() {
    vga_init();    
    idt_init();
    timer_init(100);
    pic_init();
    keyboard_init();
    pic_enable_irq(0);
    pic_enable_irq(1);
    __asm__ volatile ("sti");    
    ata_init();
    if (KN_DEBUG){
        if (ata_identify()) {
        vga_printf("ATA drive detected and ready\n");
    } else {
        vga_printf_colored(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK, "No ATA drive detected (using simulation mode)\n");
    }
    }
    
    fat16_init();
    shell_init();
    keyboard_tab_handle(shell_tab_handle);
    shell_run();
}