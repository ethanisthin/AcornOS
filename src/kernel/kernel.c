#include "vga.h"

void kernel_main() {
    vga_init();
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    
    vga_puts("AcornOS Kernel lives again!\n");
    vga_puts("Line 2: Cursor position test\n");
    vga_puts("Line 3: With newlines and tabs\tTabbed text\n");
    
    vga_set_cursor(10, 10);
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_RED);
    vga_puts("Positioned text at (10,10)");
    
    vga_set_cursor(0, 12);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("Back to normal position");
    
    while (1);
}