#ifndef VGA_H
#define VGA_H

/* Defintions */
#define VGA_COLOUR_BLACK 0
#define VGA_COLOUR_BLUE 1
#define VGA_COLOUR_GREEN 2
#define VGA_COLOUR_CYAN 3
#define VGA_COLOUR_RED 4
#define VGA_COLOUR_MAGENTA 5
#define VGA_COLOUR_BROWN 6
#define VGA_COLOUR_LIGHT_GRAY 7
#define VGA_COLOUR_DARK_GRAY 8
#define VGA_COLOUR_LIGHT_BLUE 9
#define VGA_COLOUR_LIGHT_GREEN 10
#define VGA_COLOUR_LIGHT_CYAN 11
#define VGA_COLOUR_LIGHT_RED 12
#define VGA_COLOUR_LIGHT_MAGENTA 13
#define VGA_COLOUR_LIGHT_BROWN 14
#define VGA_COLOUR_WHITE 15
#define MEM_SPACE 0xB8000

#define WIDTH 80
#define HEIGHT 25

/* Function Declarations */

unsigned char vga_colour(unsigned char fg, unsigned char bg);
unsigned short vga_entry(unsigned char c, unsigned char colour);
void clr_scr();
void set_cur(int x, int y);
void set_colour(unsigned char fg, unsigned char bg);
void scroll_up();
void enter_char(char c);
void print(const char* str);
void println(const char* str);
void printf(const char* format, ...);
void mark_inp_start();
int is_before_inp_start();
void update_cursor();

#endif