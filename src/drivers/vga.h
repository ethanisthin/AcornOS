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

typedef struct {
    vga_colours fg;
    vga_colours bg;
} vga_colour_pair;

void vga_init(void);
void vga_clear(void);
void vga_set_color(vga_colours fg, vga_colours bg);  
void vga_putchar(char c);
void vga_puts(const char* str);
void vga_putchar_at(char c, int x, int y);
void vga_set_cursor(int x, int y);  
void vga_get_cursor(int* x, int* y);  
void vga_move_cursor(int dx, int dy); 
void vga_enable_cursor(void);  
void vga_disable_cursor(void); 
void vga_scroll(void);

vga_colour_pair vga_get_color(void);
void vga_save_color(vga_colour_pair* saved); 
void vga_restore_color(const vga_colour_pair* saved); 
void vga_puts_colored(const char* str, vga_colours fg, vga_colours bg);
void vga_putchar_colored(char c, vga_colours fg, vga_colours bg);

void vga_print_line(char c, int length);
void vga_print_centered(const char* str);
void vga_print_at(const char* str, int x, int y);
void vga_clear_line(int y);
void vga_fill_rect(int x, int y, int width, int height, char c, vga_colours fg, vga_colours bg);

void vga_print_int(int value);
void vga_print_uint(unsigned int value);
void vga_print_hex(unsigned int value);
void vga_print_hex_padded(unsigned int value, int width);
void vga_printf(const char* format, ...);
void vga_printf_colored(vga_colours fg, vga_colours bg, const char* format, ...);
void vga_printf_at(int x, int y, const char* format, ...);

#endif