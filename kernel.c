/* Libraries */
#include <stddef.h>

/* Definitions */
#define WIDTH 80
#define HEIGHT 25
#define MEM_SPACE 0xB8000
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
#define IDT_ENTRIES 256
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define ICW1_ICW4 0x01
#define ICW1_SINGLE 0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL 0x08
#define ICW1_INIT 0x10
#define ICW4_8086 0x01
#define ICW4_AUTO 0x02
#define ICW4_BUF_SLAVE 0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM 0x10
#define IRQ0 32
#define IRQ1 33
#define MOD_SHIFT     (1 << 0)
#define MOD_CTRL      (1 << 1)
#define MOD_ALT       (1 << 2)
#define MOD_CAPSLOCK  (1 << 3)
#define KBD_DATA_PORT    0x60
#define KBD_STATUS_PORT  0x64
#define KBD_CMD_PORT     0x64


/* Struct Definitions */
struct idt_entry{
    unsigned short base_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char flags;
    unsigned short base_high;
} __attribute__((packed));

struct idt_descriptor {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));




/* Struct Declarations */
struct idt_entry idt[IDT_ENTRIES];
struct idt_descriptor idt_desc;



/* Global Variables */
static int cur_x = 0;
static int cur_y = 0;
static unsigned char curr_clr = 0x0F;
static unsigned char kbd_modifiers = 0;
static int inp_start_x = 0;
static int inp_start_y = 0;



/* Function Declarations */
void idt_set_gate(unsigned char num, unsigned int base, unsigned short selector, unsigned char flags);
void idt_install();
extern void idt_load();
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
void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);
void pic_remapper(int off1, int off2);
void disable_pic();
extern void irq0_handler();
extern void irq1_handler();
void kbm_handler();
void irq_handle_install(int, void(*)());
void irq_handle_uninstall(int);
void mark_inp_start();
int is_before_inp_start();







/* Main Function */
void _start() {
    __asm__ volatile("cli");
    __asm__ volatile("mov $0x90000, %esp");
    
    set_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLACK);
    clr_scr();
    idt_install();
    
    pic_remapper(0x20, 0x28);
    outb(PIC1_DATA, inb(PIC1_DATA) & ~0x02);  
    
    while (inb(KBD_STATUS_PORT) & 0x02);     
    outb(KBD_DATA_PORT, 0xF4);              

    irq_handle_install(1, kbm_handler);
    
    __asm__ volatile("sti");
    
    println("Welcome!\n");

    mark_inp_start();
    
    while(1) {
        __asm__ volatile("hlt");
    }
}

/* BASIC SCREEN FUNCTIONS */
unsigned char vga_colour(unsigned char fg, unsigned char bg){
    return fg|bg << 4;
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
}

void set_cur(int x, int y){
    if (x>=0 && x<WIDTH && y>=0 && y<HEIGHT){
        cur_x = x;
        cur_y = y;
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

/* IDT STUFF */
void idt_set_gate(unsigned char num, unsigned int base, unsigned short selector, unsigned char flags){
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

void idt_install(){
    idt_desc.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idt_desc.base = (unsigned int)&idt;

    for (int i=0; i<IDT_ENTRIES; i++){
        idt_set_gate(i,0,0,0);
    }
    idt_set_gate(IRQ0, (unsigned int)irq0_handler, 0x08, 0x8E); //timer interrupt
    idt_set_gate(IRQ1, (unsigned int)irq1_handler, 0x08, 0x8E); //kbm interrupt
    idt_load();
}


/* PIC STUFF */
void outb(unsigned short port, unsigned char val){
    __asm__ volatile("outb %0, %1": : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port){
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void pic_remapper(int off1, int off2){
    unsigned char a1, a2;
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, off1);
    outb(PIC2_DATA, off2);
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void disable_pic(){
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}


/* IRQ STUFF */

void* irq_routine[16] = {0};

void irq_handler(int irq){
    void (*handler)() = irq_routine[irq];
    if (handler){
        handler();
    }
    if (irq>=8){
        outb(PIC2_COMMAND, 0x20);
    }
    outb(PIC1_COMMAND, 0x20);
}


void kbm_handler() {
    static const unsigned char kbm_normal[128] = {
        0,    0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0,    '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',  0,
        '*',  0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,    0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,   0,
        0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0
    };

    static const unsigned char kbm_shift[128] = {
        0,    0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
        0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',  0,
        '*',  0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,    0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,   0,
        0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0
    };

    unsigned char scancode = inb(0x60);

    switch(scancode) {
        case 0x2A: 
        case 0x36: 
            kbd_modifiers |= MOD_SHIFT;
            goto end;
        case 0xAA: 
        case 0xB6: 
            kbd_modifiers &= ~MOD_SHIFT;
            goto end;
        case 0x3A:
            kbd_modifiers ^= MOD_CAPSLOCK;
            goto end;   
    }

    if (scancode < 0x80) {
        if (scancode >= sizeof(kbm_normal)) {
            goto end;
        }
        int use_shifted = (kbd_modifiers & MOD_SHIFT);
        

        if ((kbd_modifiers & MOD_CAPSLOCK) && 
            ((scancode >= 0x10 && scancode <= 0x1C) ||  
             (scancode >= 0x1E && scancode <= 0x26) ||  
             (scancode >= 0x2C && scancode <= 0x32))) { 
            use_shifted ^= 1; 
        }

        char ascii = use_shifted ? kbm_shift[scancode] : kbm_normal[scancode];
        if (ascii != 0) {
            enter_char(ascii);
        }
    }

end:
    outb(0x20, 0x20);
}

void irq_handle_install(int irq, void (*handler)()){
    irq_routine[irq]=handler;
}

void irq_handle_uninstall(int irq){
    irq_routine[irq]=0;
}


void printf(const char* format, ...) {
    const char* traverse;
    unsigned int* arg = (unsigned int*)&format;
    arg++;
    
    for (traverse = format; *traverse != '\0'; traverse++) {
        if (*traverse != '%') {
            enter_char(*traverse);
            continue;
        }
        
        traverse++;
        switch (*traverse) {
            case 'c': {
                char c = (char)*arg;
                enter_char(c);
                arg++;
                break;
            }
            case 's': {
                char* s = (char*)*arg;
                if (s==NULL) s = "(null)";
                print(s);
                arg++;
                break;
            }
            case 'd': {
                int num = *arg;
                if (num < 0) {
                    enter_char('-');
                    num = -num;
                }
                
                char buffer[32];
                int i = 0;
                if (num == 0) {
                    buffer[i++] = '0';
                } else {
                    while (num > 0) {
                        buffer[i++] = '0' + (num % 10);
                        num /= 10;
                    }
                }
                
                for (int j = i - 1; j >= 0; j--) {
                    enter_char(buffer[j]);
                }
                arg++;
                break;
            }
            case 'x': {
                unsigned int num = *arg;
                char buffer[32];
                int i = 0;
                
                if (num == 0) {
                    buffer[i++] = '0';
                } else {
                    while (num > 0) {
                        int digit = num % 16;
                        buffer[i++] = digit < 10 ? '0' + digit : 'A' + digit - 10;
                        num /= 16;
                    }
                }
                
                for (int j = i - 1; j >= 0; j--) {
                    enter_char(buffer[j]);
                }
                arg++;
                break;
            }
            default:
                enter_char(*traverse);
                break;
        }
    }
}