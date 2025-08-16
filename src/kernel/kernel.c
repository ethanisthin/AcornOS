#include "../drivers/vga.h"

void kernel_main() {
    vga_init();
    vga_set_clr(VGA_COLOR_GREEN, VGA_COLOR_BLUE);
    vga_puts("AcornOS Kernel lives again!!!!");
    while (1);
}