#ifndef EDITOR_H
#define EDITOR_H

#include "../include/kernel/types.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../filesystem/fat16.h"
#include "../lib/string/string.h"
#include <stdbool.h>

// Arrow key scancodes (add these to keyboard.h)
#define KEY_UP_ARROW    0x48
#define KEY_DOWN_ARROW  0x50  
#define KEY_LEFT_ARROW  0x4B
#define KEY_RIGHT_ARROW 0x4D
#define KEY_HOME        0x47
#define KEY_END         0x4F
#define KEY_PAGE_UP     0x49
#define KEY_PAGE_DOWN   0x51

// Editor constants
#define EDITOR_MAX_LINES 1000
#define EDITOR_MAX_LINE_LENGTH 256
#define EDITOR_SCREEN_HEIGHT 24  // Leave 1 line for status
#define EDITOR_SCREEN_WIDTH 80
#define EDITOR_STATUS_LINE 24

// Editor modes
typedef enum {
    EDITOR_MODE_COMMAND,
    EDITOR_MODE_INSERT,
    EDITOR_MODE_COMMAND_LINE
} editor_mode_t;

// Editor state
typedef struct {
    char lines[EDITOR_MAX_LINES][EDITOR_MAX_LINE_LENGTH];
    int line_count;
    int cursor_x, cursor_y;        // Cursor position in text
    int screen_x, screen_y;        // Cursor position on screen
    int scroll_offset_y;           // Vertical scroll
    int scroll_offset_x;           // Horizontal scroll
    char filename[256];
    bool modified;
    editor_mode_t mode;
    char command_buffer[256];      // For :w, :q commands
    int command_pos;
    char status_message[256];
    bool should_exit;
} editor_state_t;

extern editor_state_t editor_state;

// Function prototypes
void editor_init(void);
void editor_run(const char* filename);
void editor_handle_input(void);
void editor_handle_command_mode(uint8_t scancode, char ascii);
void editor_handle_insert_mode(uint8_t scancode, char ascii);
void editor_handle_command_line_mode(uint8_t scancode, char ascii);
void editor_cleanup(void);

// Navigation functions
void editor_move_cursor_up(void);
void editor_move_cursor_down(void);
void editor_move_cursor_left(void);
void editor_move_cursor_right(void);
void editor_move_to_line_start(void);
void editor_move_to_line_end(void);
void editor_page_up(void);
void editor_page_down(void);

// Text editing functions
void editor_insert_char(char c);
void editor_delete_char(void);
void editor_backspace(void);
void editor_insert_newline(void);
void editor_delete_line(void);

// File operations
bool editor_load_file(const char* filename);
bool editor_save_file(void);
void editor_new_file(const char* filename);

// Display functions
void editor_refresh_screen(void);
void editor_draw_status_line(void);
void editor_draw_text(void);
void editor_update_cursor_position(void);
void editor_scroll_if_needed(void);

// Command processing
void editor_process_command(const char* command);
void editor_set_status_message(const char* message);

// Shell integration
void cmd_edit(int argc, char* argv[]);

#endif