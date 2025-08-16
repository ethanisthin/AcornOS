#ifndef VGA_H
#define VGA_H

#define WIDTH 80
#define HEIGHT 25
#define MEMORY 0xB8000

typedef enum{
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
} vga_colours;

void vga_init(void);
void vga_clear(void);
void vga_set_clr(vga_colours fg, vga_colours bg);
void vga_putchar(char c);
void vga_puts(const char* str);
void vga_putchar_at(char c, int x, int y);

#endif