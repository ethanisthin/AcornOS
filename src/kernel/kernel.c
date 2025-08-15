#define VGA_COLOUR_BLACK 0x00
#define VGA_COLOUR_BLUE 0x01
#define VGA_COLOUR_GREEN 0x02
#define VGA_COLOUR_RED 0x04
#define VGA_COLOUR_WHITE 0x0F

void kernel_main() {
    volatile unsigned short *vga = (volatile unsigned short*)0xB8000;

    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = (VGA_COLOUR_BLUE << 12) | (VGA_COLOUR_WHITE << 8) | ' ';
    }

    const char *msg = "AcornOS Kernel lives again!!!!";

    for (int i = 0; msg[i]; i++) {
        vga[i] = (VGA_COLOUR_GREEN << 8) | msg[i];
    }

    while (1);
}