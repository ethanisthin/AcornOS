#include "../drivers/vga.h"
#include "../lib/string/string.h"
#include <stdarg.h>

/* Global Variables */
static volatile unsigned short* vga_buffer = (volatile unsigned short*)MEMORY;
static vga_colours current_fg = VGA_COLOR_WHITE;
static vga_colours current_bg = VGA_COLOR_BLACK;
static int cursor_x = 0;
static int cursor_y = 0;

/* Port functions */
static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline unsigned short vga_entry(char c, vga_colours fg, vga_colours bg) {
    return (unsigned short)c | ((unsigned short)(fg | (bg << 4)) << 8);
}

/* Cursor stuff */
static void update_hardware_cursor(void) {
    unsigned short pos = cursor_y * WIDTH + cursor_x;
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);
}

void vga_set_cursor(int x, int y) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        cursor_x = x;
        cursor_y = y;
        update_hardware_cursor();
    }
}

void vga_get_cursor(int* x, int* y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

void vga_move_cursor(int dx, int dy) {
    vga_set_cursor(cursor_x + dx, cursor_y + dy);
}

void vga_enable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0); 
    
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

void vga_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); 
}

/* Scroll and buffer stuff */
void vga_scroll(void) {
    for (int y = 0; y < HEIGHT - 1; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int src_index = (y + 1) * WIDTH + x;
            int dst_index = y * WIDTH + x;
            vga_buffer[dst_index] = vga_buffer[src_index];
        }
    }
    
    for (int x = 0; x < WIDTH; x++) {
        int index = (HEIGHT - 1) * WIDTH + x;
        vga_buffer[index] = vga_entry(' ', current_fg, current_bg);
    }
}

void vga_init(void) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_clear();
    vga_set_cursor(0, 0);
    vga_enable_cursor();
}

void vga_clear(void) {
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', current_fg, current_bg);
    }
    vga_set_cursor(0, 0);
}

void vga_clear_line(int y) {
    if (y >= 0 && y < HEIGHT) {
        for (int x = 0; x < WIDTH; x++) {
            int index = y * WIDTH + x;
            vga_buffer[index] = vga_entry(' ', current_fg, current_bg);
        }
    }
}

void vga_fill_rect(int x, int y, int width, int height, char c, vga_colours fg, vga_colours bg) {
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
                int index = py * WIDTH + px;
                vga_buffer[index] = vga_entry(c, fg, bg);
            }
        }
    }
}

/* Colour stuff */
void vga_set_color(vga_colours fg, vga_colours bg) {
    current_fg = fg;
    current_bg = bg;
}

vga_colour_pair vga_get_color(void) {
    vga_colour_pair pair = {current_fg, current_bg};
    return pair;
}

void vga_save_color(vga_colour_pair* saved) {
    saved->fg = current_fg;
    saved->bg = current_bg;
}

void vga_restore_color(const vga_colour_pair* saved) {
    current_fg = saved->fg;
    current_bg = saved->bg;
}

/*Char stuff*/
void vga_putchar_at(char c, int x, int y) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        int index = y * WIDTH + x;
        vga_buffer[index] = vga_entry(c, current_fg, current_bg);
    }
}

void vga_putchar_colored(char c, vga_colours fg, vga_colours bg) {
    vga_colour_pair saved;
    vga_save_color(&saved);
    vga_set_color(fg, bg);
    vga_putchar(c);
    vga_restore_color(&saved);
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7; 
    } else if (c == '\b') {  
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = WIDTH - 1;
        }
        vga_putchar_at(' ', cursor_x, cursor_y);
    } else if (c >= 32) { 
        vga_putchar_at(c, cursor_x, cursor_y);
        cursor_x++;
    }
    
    if (cursor_x >= WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= HEIGHT) {
        vga_scroll();
        cursor_y = HEIGHT - 1;
        cursor_x = 0;
    }
    
    update_hardware_cursor();
}

/* String helpers */
void vga_puts(const char* str) {
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}

void vga_puts_colored(const char* str, vga_colours fg, vga_colours bg) {
    vga_colour_pair saved;
    vga_save_color(&saved);
    vga_set_color(fg, bg);
    vga_puts(str);
    vga_restore_color(&saved);
}

void vga_print_at(const char* str, int x, int y) {
    int saved_x = cursor_x;
    int saved_y = cursor_y;
    vga_set_cursor(x, y);
    vga_puts(str);
    vga_set_cursor(saved_x, saved_y);
}

void vga_print_centered(const char* str) {
    int len = strlen(str);
    int x = (WIDTH - len) / 2;
    if (x < 0) x = 0;
    
    int saved_x = cursor_x;
    cursor_x = x;
    vga_puts(str);
    cursor_x = saved_x;
}

void vga_print_line(char c, int length) {
    for (int i = 0; i < length && i < WIDTH; i++) {
        vga_putchar(c);
    }
}


/* Num prints */
void vga_print_int(int value) {
    char buffer[32];
    int_to_string(value, buffer, 10);
    vga_puts(buffer);
}

void vga_print_uint(unsigned int value) {
    char buffer[32];
    uint_to_string(value, buffer, 10);
    vga_puts(buffer);
}

void vga_print_hex(unsigned int value) {
    char buffer[32];
    uint_to_string(value, buffer, 16);
    vga_puts(buffer);
}

void vga_print_hex_padded(unsigned int value, int width) {
    char buffer[32];
    int len = uint_to_string(value, buffer, 16);
    for (int i = len; i < width; i++) {
        vga_putchar('0');
    }
    vga_puts(buffer);
}


/* Print formatting */
void vga_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    vga_print_int(value);
                    break;
                }
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    vga_print_uint(value);
                    break;
                }
                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    vga_print_hex(value);
                    break;
                }
                case 'X': {
                    unsigned int value = va_arg(args, unsigned int);
                    vga_puts("0x");
                    vga_print_hex(value);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vga_putchar(c);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (str) {
                        vga_puts(str);
                    } else {
                        vga_puts("(null)");
                    }
                    break;
                }
                case '%': {
                    vga_putchar('%');
                    break;
                }
                default: {
                    vga_putchar('%');
                    vga_putchar(*format);
                    break;
                }
            }
        } else {
            vga_putchar(*format);
        }
        format++;
    }
    
    va_end(args);
}

void vga_printf_colored(vga_colours fg, vga_colours bg, const char* format, ...) {
    vga_colour_pair saved;
    vga_save_color(&saved);
    vga_set_color(fg, bg);
    
    va_list args;
    va_start(args, format);

    const char* fmt = format;
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    vga_print_int(value);
                    break;
                }
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    vga_print_uint(value);
                    break;
                }
                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    vga_print_hex(value);
                    break;
                }
                case 'X': {
                    unsigned int value = va_arg(args, unsigned int);
                    vga_puts("0x");
                    vga_print_hex(value);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vga_putchar(c);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (str) {
                        vga_puts(str);
                    } else {
                        vga_puts("(null)");
                    }
                    break;
                }
                case '%': {
                    vga_putchar('%');
                    break;
                }
                default: {
                    vga_putchar('%');
                    vga_putchar(*fmt);
                    break;
                }
            }
        } else {
            vga_putchar(*fmt);
        }
        fmt++;
    }
    
    va_end(args);
    vga_restore_color(&saved);
}

void vga_printf_at(int x, int y, const char* format, ...) {
    int saved_x, saved_y;
    vga_get_cursor(&saved_x, &saved_y);
    vga_set_cursor(x, y);
    
    va_list args;
    va_start(args, format);
    
    const char* fmt = format;
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    vga_print_int(value);
                    break;
                }
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    vga_print_uint(value);
                    break;
                }
                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    vga_print_hex(value);
                    break;
                }
                case 'X': {
                    unsigned int value = va_arg(args, unsigned int);
                    vga_puts("0x");
                    vga_print_hex(value);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vga_putchar(c);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (str) {
                        vga_puts(str);
                    } else {
                        vga_puts("(null)");
                    }
                    break;
                }
                case '%': {
                    vga_putchar('%');
                    break;
                }
                default: {
                    vga_putchar('%');
                    vga_putchar(*fmt);
                    break;
                }
            }
        } else {
            vga_putchar(*fmt);
        }
        fmt++;
    }
    
    va_end(args);
    vga_set_cursor(saved_x, saved_y);
}