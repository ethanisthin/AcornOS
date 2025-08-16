#include "keyboard.h"
#include "vga.h"
#include <stdbool.h>


static bool keyboard_initialized = false;
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool caps_lock = false;
static keyboard_buffer_t kb_buffer;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}


static const char scancode_to_ascii_lower[128] = {
    0,    0,   '1', '2', '3', '4', '5', '6',     
    '7',  '8', '9', '0', '-', '=', '\b', '\t',   
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',    
    'o',  'p', '[', ']', '\n', 0,  'a', 's',    
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';',    
    '\'', '`', 0,   '\\','z', 'x', 'c', 'v',    
    'b',  'n', 'm', ',', '.', '/', 0,   '*',    
    0,    ' ', 0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   '7',    
    '8',  '9', '-', '4', '5', '6', '+', '1',    
    '2',  '3', '0', '.', 0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0       
};

static const char scancode_to_ascii_upper[128] = {
    0,    0,   '!', '@', '#', '$', '%', '^',     
    '&',  '*', '(', ')', '_', '+', '\b', '\t',   
    'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',    
    'O',  'P', '{', '}', '\n', 0,  'A', 'S',    
    'D',  'F', 'G', 'H', 'J', 'K', 'L', ':',    
    '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',    
    'B',  'N', 'M', '<', '>', '?', 0,   '*',    
    0,    ' ', 0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   '7',    
    '8',  '9', '-', '4', '5', '6', '+', '1',    
    '2',  '3', '0', '.', 0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0,      
    0,    0,   0,   0,   0,   0,   0,   0       
};

void keyboard_buffer_init(void) {
    kb_buffer.read_pos = 0;
    kb_buffer.write_pos = 0;
    kb_buffer.count = 0;
}

bool keyboard_buffer_put(char c) {
    if (keyboard_buffer_is_full()) {
        return false;  
    }
    
    kb_buffer.buffer[kb_buffer.write_pos] = c;
    kb_buffer.write_pos = (kb_buffer.write_pos + 1) % KEYBOARD_BUFFER;
    kb_buffer.count++;
    return true;
}

char keyboard_buffer_get(void) {
    if (keyboard_buffer_is_empty()) {
        return 0;  
    }
    
    char c = kb_buffer.buffer[kb_buffer.read_pos];
    kb_buffer.read_pos = (kb_buffer.read_pos + 1) % KEYBOARD_BUFFER;
    kb_buffer.count--;
    return c;
}

bool keyboard_buffer_is_empty(void) {
    return kb_buffer.count == 0;
}

bool keyboard_buffer_is_full(void) {
    return kb_buffer.count >= KEYBOARD_BUFFER;
}

uint32_t keyboard_buffer_count(void) {
    return kb_buffer.count;
}

void keyboard_buffer_clear(void) {
    kb_buffer.read_pos = 0;
    kb_buffer.write_pos = 0;
    kb_buffer.count = 0;
}


static void keyboard_wait_input(void) {
    while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_INPUT_FULL) {

    }
}

static void keyboard_wait_output(void) {
    while (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL)) {

    }
}


void keyboard_gets(char* buffer, uint32_t max_length) {
    uint32_t pos = 0;
    char c;
    
    while (pos < max_length - 1) {
        c = keyboard_getchar();
        
        if (c == '\n') {
            break; 
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                vga_printf("\b \b");  
            }
        } else if (c >= 32 && c <= 126) {
            buffer[pos] = c;
            pos++;
            vga_printf("%c", c);
        }
    }
    
    buffer[pos] = '\0';
    vga_printf("\n");
}



char keyboard_getchar(void) {
    while (keyboard_buffer_is_empty()) {
        __asm__ volatile ("hlt");  
    }
    return keyboard_buffer_get();
}

bool keyboard_has_input(void) {
    return !keyboard_buffer_is_empty();
}

uint8_t keyboard_read_scancode(void) {
    keyboard_wait_output();
    return inb(KEYBOARD_DATA_PORT);
}


bool keyboard_is_key_pressed(uint8_t scancode) {
    return (scancode & 0x80) == 0;
}




char keyboard_get_ascii_char(uint8_t scancode, bool shift_pressed) {
    if (scancode >= 128) {
        return 0;  
    }
    
    bool use_upper = shift_pressed;
    if (caps_lock && scancode >= 0x10 && scancode <= 0x19) { 
        use_upper = !use_upper;
    }
    if (caps_lock && scancode >= 0x1E && scancode <= 0x26) { 
        use_upper = !use_upper;
    }
    if (caps_lock && scancode >= 0x2C && scancode <= 0x32) { 
        use_upper = !use_upper;
    }
    
    if (use_upper) {
        return scancode_to_ascii_upper[scancode];
    } else {
        return scancode_to_ascii_lower[scancode];
    }
}

key_result_t keyboard_scancode_to_ascii(uint8_t scancode) {
    key_result_t result = {0, false, false};
    
    switch (scancode) {
        case KEY_ESCAPE:
        case KEY_BACKSPACE:
        case KEY_TAB:
        case KEY_ENTER:
            result.ascii = keyboard_get_ascii_char(scancode, shift_pressed);
            result.is_printable = (result.ascii != 0);
            result.is_special = true;
            return result;
            
        case KEY_LSHIFT:
        case KEY_RSHIFT:
        case KEY_LCTRL:
        case KEY_LALT:
            result.is_special = true;
            return result;
            
        case KEY_CAPSLOCK:
            result.is_special = true;
            return result;
            
        default:
            result.ascii = keyboard_get_ascii_char(scancode, shift_pressed);
            result.is_printable = (result.ascii != 0);
            result.is_special = false;
            return result;
    }
}


void keyboard_handler(void) {
    uint8_t scancode = keyboard_read_scancode();
    
    if (keyboard_is_key_pressed(scancode)) {
        switch (scancode) {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                shift_pressed = true;
                return;
                
            case KEY_LCTRL:
                ctrl_pressed = true;
                return;
                
            case KEY_LALT:
                alt_pressed = true;
                return;
                
            case KEY_CAPSLOCK:
                caps_lock = !caps_lock;
                return;
        }
    
        uint8_t modifiers = keyboard_get_modifiers();
        if (modifiers & (KEY_MOD_CTRL | KEY_MOD_ALT)) {
            keyboard_handle_special_combination(scancode, modifiers);
            return;  
        }
        key_result_t key = keyboard_scancode_to_ascii(scancode);
        if (key.is_printable) {
            if (!keyboard_buffer_put(key.ascii)) {
                vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK, "[BUFFER FULL]");
            }
        }
        
    } else {
        uint8_t released_key = scancode & 0x7F;
        switch (released_key) {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                shift_pressed = false;
                break;
                
            case KEY_LCTRL:
                ctrl_pressed = false;
                break;
                
            case KEY_LALT:
                alt_pressed = false;
                break;
        }
    }
}

keyboard_state_t keyboard_get_state(void) {
    keyboard_state_t state;
    state.shift_pressed = shift_pressed;
    state.ctrl_pressed = ctrl_pressed;
    state.alt_pressed = alt_pressed;
    state.caps_lock = caps_lock;
    state.modifier_flags = 0;
    
    if (shift_pressed) state.modifier_flags |= KEY_MOD_SHIFT;
    if (ctrl_pressed) state.modifier_flags |= KEY_MOD_CTRL;
    if (alt_pressed) state.modifier_flags |= KEY_MOD_ALT;
    if (caps_lock) state.modifier_flags |= KEY_MOD_CAPS;
    
    return state;
}

bool keyboard_is_shift_pressed(void) {
    return shift_pressed;
}

bool keyboard_is_ctrl_pressed(void) {
    return ctrl_pressed;
}

bool keyboard_is_alt_pressed(void) {
    return alt_pressed;
}

bool keyboard_is_caps_lock_on(void) {
    return caps_lock;
}

uint8_t keyboard_get_modifiers(void) {
    uint8_t modifiers = 0;
    if (shift_pressed) modifiers |= KEY_MOD_SHIFT;
    if (ctrl_pressed) modifiers |= KEY_MOD_CTRL;
    if (alt_pressed) modifiers |= KEY_MOD_ALT;
    if (caps_lock) modifiers |= KEY_MOD_CAPS;
    return modifiers;
}

void keyboard_handle_special_combination(uint8_t scancode, uint8_t modifiers) {
    if (modifiers & KEY_MOD_CTRL) {
        switch (scancode) {
            case 0x2E: 
                vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK, "^C");
                break;
            case 0x2D: 
                vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK, "^X");
                break;
            case 0x2F: 
                vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK, "^V");
                break;
            case 0x1F: 
                vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK, "^S");
                break;
        }
    }
    
    if (modifiers & KEY_MOD_ALT) {
        vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK, "[Alt+%02X]", scancode);
    }
}


void keyboard_init(void) {
    vga_printf("Initializing keyboard...\n");
    keyboard_buffer_init();
    keyboard_wait_input();
    outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_DISABLE_KEYBOARD);
    
    while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL) {
        inb(KEYBOARD_DATA_PORT);
    }
    
    keyboard_wait_input();
    outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_READ_CONFIG);
    keyboard_wait_output();
    uint8_t config = inb(KEYBOARD_DATA_PORT);
    
    config |= 0x01;   
    config &= ~0x20; 
    
    keyboard_wait_input();
    outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_WRITE_CONFIG);
    keyboard_wait_input();
    outb(KEYBOARD_DATA_PORT, config);
    
    keyboard_wait_input();
    outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_ENABLE_KEYBOARD);
    
    keyboard_initialized = true;
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "Keyboard initialized successfully!\n");
}