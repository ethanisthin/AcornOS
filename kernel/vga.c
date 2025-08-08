#include "vga.h"
#include "kernel.h"
#include "io.h"

int cur_x = 0;
int cur_y = 0;
unsigned char curr_clr = 0x0F;
int inp_start_x = 0;
int inp_start_y = 0;

/* BASIC SCREEN FUNCTIONS */
unsigned char vga_colour(unsigned char fg, unsigned char bg){
    return (fg&0x0F)|((bg&0x0F) << 4);
}

unsigned short vga_entry(unsigned char c, unsigned char colour){
    return (unsigned short) c | (unsigned short) colour << 8;
}

void clr_scr(){
    unsigned short* vid_mem = (unsigned short*)MEM_SPACE;
    unsigned short blank = vga_entry(' ', curr_clr);

    for (int i=0; i < WIDTH*HEIGHT; i++){
        vid_mem[i]=blank;
    }

    cur_x=0;
    cur_y=0;
    update_cursor();
}

void set_cur(int x, int y){
    if (x>=0 && x<WIDTH && y>=0 && y<HEIGHT){
        cur_x = x;
        cur_y = y;
        update_cursor();
    }
}

void set_colour(unsigned char fg, unsigned char bg){
    curr_clr = vga_colour(fg,bg);
}

void scroll_up(){
    unsigned short* vid_mem = (unsigned short*)MEM_SPACE;
    unsigned short blank = vga_entry(' ', curr_clr);

    for (int i=0; i<(HEIGHT-1)*WIDTH; i++){
        vid_mem[i] = vid_mem[i+WIDTH];
    } 
    for (int i=(HEIGHT-1)*WIDTH; i<HEIGHT*WIDTH; i++){
        vid_mem[i] = blank;
    }

    cur_y = HEIGHT-1;
    update_cursor();
}

void enter_char(char c){
    unsigned short* vid_mem = (unsigned short*)MEM_SPACE;
    
    if (c == '\n') {
        cur_x = 0;
        cur_y++;
    } else if (c == '\r') {
        cur_x = 0;
    } else if (c == '\t') {
        cur_x = (cur_x + 8) & ~(8 - 1);
    } else if (c == '\b') {
        if (cur_x > 0) {
            int x_temp = cur_x - 1;
            int y_temp = cur_y;
            if (y_temp > inp_start_y || (y_temp == inp_start_y && x_temp >= inp_start_x)){
                cur_x--;
                vid_mem[cur_y*WIDTH + cur_x] = vga_entry(' ', curr_clr);
            }
            
        } else if (cur_y > 0){
            int y_temp = cur_y - 1;
            if (y_temp > inp_start_y || (y_temp == inp_start_y)){
                cur_y--;
                cur_x = WIDTH - 1;
                while (cur_x > 0 &&  (vid_mem[cur_y*WIDTH+cur_x] & 0xFF) == ' '){
                    cur_x--;
                }
                cur_x++;
                if (cur_x >= WIDTH){
                    cur_x = WIDTH - 1;
                }
                if (cur_y == inp_start_y && cur_x < inp_start_x){
                    cur_x = inp_start_x;
                }
            }
        }
    } else {
        vid_mem[cur_y * WIDTH + cur_x] = vga_entry(c, curr_clr);
        cur_x++;
    }
    
    if (cur_x >= WIDTH) {
        cur_x = 0;
        cur_y++;
    }
    
    if (cur_y >= HEIGHT) {
        scroll_up();
        if (inp_start_y > 0){
            inp_start_y--;
        }
    }
    update_cursor();
}

void print(const char* str) {
    while (*str) {
        enter_char(*str);
        str++;
    }
}

void println(const char* str) {
    print(str);
    enter_char('\n');
}

void mark_inp_start(){
    inp_start_x = cur_x;
    inp_start_y = cur_y;
    update_cursor();
}

int is_before_inp_start(){
    if (cur_y < inp_start_y){
        return 1;
    }

    if (cur_y == inp_start_y && cur_x <= inp_start_x){
        return 1;
    }
    return 0;
}

void update_cursor() {
    unsigned short pos = cur_y * WIDTH + cur_x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}