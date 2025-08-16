#include "../drivers/vga.h"

void kernel_main() {
    vga_init();
    vga_printf("AcornOS - Printf Test\n");
    vga_printf("===================\n\n");
    
    vga_printf("Integer: %d\n", 42);
    vga_printf("Negative: %d\n", -123);
    vga_printf("Unsigned: %u\n", 4294967295U);
    vga_printf("Hex: %x\n", 255);
    vga_printf("Hex with 0x: %X\n", 255);
    vga_printf("Character: %c\n", 'A');
    vga_printf("String: %s\n", "Hello, World!");
    vga_printf("Percent: %%\n\n");
    
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, 
                      "Colored text: %s = %d\n", "Answer", 42);
    vga_printf_at(10, 15, "Positioned: x=%d, y=%d", 10, 15);
    vga_set_cursor(0, 17);
    vga_printf("VGA Buffer at: %X\n", MEMORY);
    vga_printf("Kernel function at: %X\n", (unsigned int)kernel_main);
    vga_printf("Screen: %dx%d, Colors: %d\n", WIDTH, HEIGHT, 16);
    vga_printf_colored(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
                      "\nAll printf tests complete!\n");
    
    while (1);
}