#include "vga.h"


static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static volatile unsigned short* vga_buffer = (volatile unsigned short*)MEMORY;
static vga_colours current_fg = VGA_COLOR_WHITE;
static vga_colours current_bg = VGA_COLOR_BLACK;
static int cursor_x = 0;
static int cursor_y = 0;

static inline unsigned short vga_entry(char c, vga_colours fg, vga_colours bg) {
    return (unsigned short)c | ((unsigned short)(fg | (bg << 4)) << 8);
}

static void update_hardware_cursor(void) {
    unsigned short pos = cursor_y * WIDTH + cursor_x;
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);

    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);
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

void vga_set_color(vga_colours fg, vga_colours bg) {
    current_fg = fg;
    current_bg = bg;
}

void vga_putchar_at(char c, int x, int y) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        int index = y * WIDTH + x;
        vga_buffer[index] = vga_entry(c, current_fg, current_bg);
    }
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7; 
    } else if (c >= 32) { 
        vga_putchar_at(c, cursor_x, cursor_y);
        cursor_x++;
    }
    
    if (cursor_x >= WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= HEIGHT) {
        cursor_y = HEIGHT - 1;
    }
    
    update_hardware_cursor();
}

void vga_puts(const char* str) {
    while (*str) {
        vga_putchar(*str);
        str++;
    }
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