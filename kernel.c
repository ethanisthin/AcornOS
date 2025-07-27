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

static int cur_x = 0;
static int cur_y = 0;
static unsigned char curr_clr = 0x0F;

void _start() {
    __asm__ volatile("mov $0x90000, %esp");
    
    set_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLACK);
    clr_scr();
    
    println("AcornOS Kernel");
    println("========================================");
    println("");
    
    set_colour(VGA_COLOUR_LIGHT_GREEN, VGA_COLOUR_BLACK);
    println("Testing colors and formatting:");
    
    set_colour(VGA_COLOUR_LIGHT_RED, VGA_COLOUR_BLACK);
    print("Red text ");
    
    set_colour(VGA_COLOUR_LIGHT_BLUE, VGA_COLOUR_BLACK);
    print("Blue text ");
    
    set_colour(VGA_COLOUR_MAGENTA, VGA_COLOUR_BLACK);
    println("Yellow text");
    
    set_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLACK);
    println("");
    
    println("Testing printf functionality:");
    printf("Character: %c\n", 'A');
    printf("String: %s\n", "This is some text on this screen");
    printf("Decimal: %d\n", 42);
    printf("Hexadecimal: %x\n", 255);
    
    println("");
    println("Testing cursor positioning:");
    set_cur(10, 15);
    print("This text is at position (10, 15)");

    set_cur(0, 20);
    println("Back to column 0, row 20");
    
    while(1) {
        __asm__ volatile("hlt");
    }
}

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
            cur_x--;
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