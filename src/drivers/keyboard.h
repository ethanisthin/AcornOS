#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../include/kernel/types.h"
#include <stdbool.h>

#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64
#define KEYBOARD_STATUS_OUTPUT_FULL  0x01
#define KEYBOARD_STATUS_INPUT_FULL   0x02
#define KEYBOARD_CMD_READ_CONFIG     0x20
#define KEYBOARD_CMD_WRITE_CONFIG    0x60
#define KEYBOARD_CMD_DISABLE_MOUSE   0xA7
#define KEYBOARD_CMD_ENABLE_MOUSE    0xA8
#define KEYBOARD_CMD_TEST_MOUSE      0xA9
#define KEYBOARD_CMD_SELF_TEST       0xAA
#define KEYBOARD_CMD_TEST_KEYBOARD   0xAB
#define KEYBOARD_CMD_DISABLE_KEYBOARD 0xAD
#define KEYBOARD_CMD_ENABLE_KEYBOARD 0xAE

#define KEY_ESCAPE    0x01
#define KEY_BACKSPACE 0x0E
#define KEY_TAB       0x0F
#define KEY_ENTER     0x1C
#define KEY_LCTRL     0x1D
#define KEY_LSHIFT    0x2A
#define KEY_RSHIFT    0x36
#define KEY_LALT      0x38
#define KEY_SPACE     0x39
#define KEY_CAPSLOCK  0x3A
#define KEY_F1        0x3B
#define KEY_F2        0x3C
#define KEY_F3        0x3D
#define KEY_F4        0x3E
#define KEY_F5        0x3F
#define KEY_F6        0x40
#define KEY_F7        0x41
#define KEY_F8        0x42
#define KEY_F9        0x43
#define KEY_F10       0x44

#define KEYBOARD_BUFFER 256

#define KEY_MOD_SHIFT    0x01
#define KEY_MOD_CTRL     0x02
#define KEY_MOD_ALT      0x04
#define KEY_MOD_CAPS     0x08

typedef struct {
    char ascii;
    bool is_printable;
    bool is_special;
    uint8_t modifiers;
    uint8_t scancode;
} key_result_t;

typedef struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool caps_lock;
    uint8_t modifier_flags;
} keyboard_state_t;

typedef struct {
    char buffer[KEYBOARD_BUFFER];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
} keyboard_buffer_t;

void keyboard_init(void);
void keyboard_handler(void);
uint8_t keyboard_read_scancode(void);
bool keyboard_is_key_pressed(uint8_t scancode);
key_result_t keyboard_scancode_to_ascii(uint8_t scancode);
char keyboard_get_ascii_char(uint8_t scancode, bool shift_pressed);

void keyboard_buffer_init(void);
bool keyboard_buffer_put(char c);
char keyboard_buffer_get(void);
bool keyboard_buffer_is_empty(void);
bool keyboard_buffer_is_full(void);
uint32_t keyboard_buffer_count(void);
void keyboard_buffer_clear(void);

char keyboard_getchar(void);
bool keyboard_has_input(void);
void keyboard_gets(char* buffer, uint32_t max_length);

keyboard_state_t keyboard_get_state(void);
bool keyboard_is_shift_pressed(void);
bool keyboard_is_ctrl_pressed(void);
bool keyboard_is_alt_pressed(void);
bool keyboard_is_caps_lock_on(void);
uint8_t keyboard_get_modifiers(void);
void keyboard_handle_special_combination(uint8_t scancode, uint8_t modifiers);

#endif