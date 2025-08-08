#include "kbm.h"
#include "vga.h"
#include "io.h"
#include "shell.h"

static unsigned char kbd_modifiers = 0;

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

void kbm_handler() {
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
        if (scancode >= 128) {
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
            shell_process_char(ascii);
        }
    }

end:
    return; 
}