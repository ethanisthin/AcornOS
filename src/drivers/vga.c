#include "vga.h"

static volatile unsigned short* vga_buffer = (volatile unsigned short*)MEMORY;
static vga_colours curr_fg = VGA_COLOR_WHITE;
static vga_colours curr_bg = VGA_COLOR_BLACK;

static inline unsigned short vga_entry(char c, vga_colours fg, vga_colours bg){
    return (unsigned short)c | ((unsigned short)(fg | bg << 4) << 8);
}

void vga_init(void){
    vga_set_clr(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_clear(void){
    for (int i=0; i<WIDTH*HEIGHT; i++){
        vga_buffer[i] = vga_entry(' ',curr_fg, curr_bg);
    }
}

void vga_set_clr(vga_colours fg, vga_colours bg){
    curr_fg = fg;
    curr_bg = bg;
}

void vga_putchar_at(char c, int x, int y){
    if (x>=0 && x<WIDTH && y>=0 && y<HEIGHT){
        int idx = y*WIDTH+x;
        vga_buffer[idx] = vga_entry(c, curr_fg, curr_bg);
    }
}

void vga_putchar(char c){
    vga_putchar_at(c, 0, 0);
}

void vga_puts(const char* str){
    int x=0;
    while (*str && x<WIDTH){
        vga_putchar_at(*str, x, 0);
        str++;
        x++;
    }
}