#include "../drivers/keyboard.h"
#include "editor.h"

editor_state_t editor_state;
static char file_load_buffer[8192];  
static char file_save_buffer[8192];

void editor_init(void) {
    editor_state.line_count = 1;
    editor_state.cursor_x = 0;
    editor_state.cursor_y = 0;
    editor_state.screen_x = 0;
    editor_state.screen_y = 0;
    editor_state.scroll_offset_y = 0;
    editor_state.scroll_offset_x = 0;
    editor_state.modified = false;
    editor_state.mode = EDITOR_MODE_COMMAND;
    editor_state.command_pos = 0;
    editor_state.filename[0] = '\0';
    editor_state.command_buffer[0] = '\0';
    editor_state.status_message[0] = '\0';
    editor_state.lines[0][0] = '\0';
    
    for (int i = 1; i < EDITOR_MAX_LINES; i++) {
        editor_state.lines[i][0] = '\0';
    }
    editor_state.should_exit = false;
    
}

void editor_run(const char* filename) {
    editor_init();
    
    if (filename && strlen(filename) > 0) {
        strcpy(editor_state.filename, filename);
        if (!editor_load_file(filename)) {
            editor_new_file(filename);
        }
    } else {
        editor_new_file("untitled");
    }
    
    editor_refresh_screen();
    
    while (!editor_state.should_exit) {
        editor_handle_input();
    }

    editor_cleanup();
}

void editor_cleanup(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            vga_putchar_at(' ', x, y);
        }
    }

    vga_set_cursor(0, 0);
    vga_enable_cursor();
    vga_puts("\n");
}



void editor_handle_input(void) {
    if (!keyboard_has_input()) {
        return;
    }
    
    uint8_t scancode = keyboard_read_scancode();
    key_result_t key = keyboard_scancode_to_ascii(scancode);
    
    switch (editor_state.mode) {
        case EDITOR_MODE_COMMAND:
            editor_handle_command_mode(scancode, key.ascii);
            break;
        case EDITOR_MODE_INSERT:
            editor_handle_insert_mode(scancode, key.ascii);
            break;
        case EDITOR_MODE_COMMAND_LINE:
            editor_handle_command_line_mode(scancode, key.ascii);
            break;
    }
    
    editor_refresh_screen();
}


void editor_handle_command_mode(uint8_t scancode, char ascii) {
    switch (scancode) {
        
        case KEY_UP_ARROW:
            editor_move_cursor_up();
            break;
        case KEY_DOWN_ARROW:
            editor_move_cursor_down();
            break;
        case KEY_LEFT_ARROW:
            editor_move_cursor_left();
            break;
        case KEY_RIGHT_ARROW:
            editor_move_cursor_right();
            break;
        case KEY_HOME:
            editor_move_to_line_start();
            break;
        case KEY_END:
            editor_move_to_line_end();
            break;
        case KEY_PAGE_UP:
            editor_page_up();
            break;
        case KEY_PAGE_DOWN:
            editor_page_down();
            break;
            
        default:
            if (ascii) {
                switch (ascii) {
                    case 'i':  
                        editor_state.mode = EDITOR_MODE_INSERT;
                        editor_set_status_message("-- INSERT --");
                        break;
                    case 'a':  
                        editor_move_cursor_right();
                        editor_state.mode = EDITOR_MODE_INSERT;
                        editor_set_status_message("-- INSERT --");
                        break;
                    case 'o':  
                        editor_move_to_line_end();
                        editor_insert_newline();
                        editor_state.mode = EDITOR_MODE_INSERT;
                        editor_set_status_message("-- INSERT --");
                        break;
                    case 'O':  
                        editor_move_to_line_start();
                        editor_insert_newline();
                        editor_move_cursor_up();
                        editor_state.mode = EDITOR_MODE_INSERT;
                        editor_set_status_message("-- INSERT --");
                        break;
                    case 'x':  
                        editor_delete_char();
                        break;
                    case 'd':  
                        editor_delete_line();
                        break;
                    case ':':  
                        editor_state.mode = EDITOR_MODE_COMMAND_LINE;
                        editor_state.command_pos = 0;
                        editor_state.command_buffer[0] = '\0';
                        break;
                }
            }
            break;
    }
}


void editor_handle_insert_mode(uint8_t scancode, char ascii) {
    switch (scancode) {
        case KEY_ESCAPE:
            editor_state.mode = EDITOR_MODE_COMMAND;
            editor_set_status_message("");
            break;
        case KEY_UP_ARROW:
            editor_move_cursor_up();
            break;
        case KEY_DOWN_ARROW:
            editor_move_cursor_down();
            break;
        case KEY_LEFT_ARROW:
            editor_move_cursor_left();
            break;
        case KEY_RIGHT_ARROW:
            editor_move_cursor_right();
            break;   
        case KEY_BACKSPACE:
            editor_backspace();
            break;  
        case KEY_ENTER:
            editor_insert_newline();
            break;
        default:
            if (ascii && ascii >= 32 && ascii <= 126) {  
                editor_insert_char(ascii);
            }
            break;
    }
}


void editor_handle_command_line_mode(uint8_t scancode, char ascii) {
    switch (scancode) {
        case KEY_ESCAPE:
            editor_state.mode = EDITOR_MODE_COMMAND;
            editor_state.command_buffer[0] = '\0';  
            editor_state.command_pos = 0;
            break;
            
        case KEY_ENTER:
            if (editor_state.command_pos > 0) {  
                editor_process_command(editor_state.command_buffer);
            }
            editor_state.mode = EDITOR_MODE_COMMAND;
            editor_state.command_buffer[0] = '\0';  
            editor_state.command_pos = 0;
            break;
            
        case KEY_BACKSPACE:
            if (editor_state.command_pos > 0) {
                editor_state.command_pos--;
                editor_state.command_buffer[editor_state.command_pos] = '\0';
            }
            break;
            
        default:
            if (ascii && ascii >= 32 && ascii <= 126 && 
                editor_state.command_pos < sizeof(editor_state.command_buffer) - 1) {
                editor_state.command_buffer[editor_state.command_pos] = ascii;
                editor_state.command_pos++;
                editor_state.command_buffer[editor_state.command_pos] = '\0';
            }
            break;
    }
}


void editor_move_cursor_up(void) {
    if (editor_state.cursor_y > 0) {
        editor_state.cursor_y--;
        int line_len = strlen(editor_state.lines[editor_state.cursor_y]);
        if (editor_state.cursor_x > line_len) {
            editor_state.cursor_x = line_len;
        }
        editor_scroll_if_needed();
    }
}

void editor_move_cursor_down(void) {
    if (editor_state.cursor_y < editor_state.line_count - 1) {
        editor_state.cursor_y++;
        
        int line_len = strlen(editor_state.lines[editor_state.cursor_y]);
        if (editor_state.cursor_x > line_len) {
            editor_state.cursor_x = line_len;
        }
        editor_scroll_if_needed();
    }
}

void editor_move_cursor_left(void) {
    if (editor_state.cursor_x > 0) {
        editor_state.cursor_x--;
    } else if (editor_state.cursor_y > 0) {
        
        editor_state.cursor_y--;
        editor_state.cursor_x = strlen(editor_state.lines[editor_state.cursor_y]);
        editor_scroll_if_needed();
    }
}

void editor_move_cursor_right(void) {
    int line_len = strlen(editor_state.lines[editor_state.cursor_y]);
    if (editor_state.cursor_x < line_len) {
        editor_state.cursor_x++;
    } else if (editor_state.cursor_y < editor_state.line_count - 1) {
        
        editor_state.cursor_y++;
        editor_state.cursor_x = 0;
        editor_scroll_if_needed();
    }
}

void editor_page_up(void) {
    for (int i = 0; i < EDITOR_SCREEN_HEIGHT && editor_state.cursor_y > 0; i++) {
        editor_move_cursor_up();
    }
}

void editor_page_down(void) {
    for (int i = 0; i < EDITOR_SCREEN_HEIGHT && 
         editor_state.cursor_y < editor_state.line_count - 1; i++) {
        editor_move_cursor_down();
    }
}


bool editor_load_file(const char* filename) {
    uint32_t file_size = fat16_get_file_size(filename);
    if (file_size == 0) {
        return false;  
    }
    if (file_size >= sizeof(file_load_buffer)) {
        return false;  
    }

    char* buffer = file_load_buffer;
    uint32_t bytes_read;

    if (!fat16_read_file_content(filename, buffer, file_size, &bytes_read)) {
        return false;
    }
    
    buffer[bytes_read] = '\0';
    editor_state.line_count = 0;
    int line_pos = 0;
    
    for (uint32_t i = 0; i <= bytes_read && editor_state.line_count < EDITOR_MAX_LINES; i++) {
        if (buffer[i] == '\n' || buffer[i] == '\0') {
            editor_state.lines[editor_state.line_count][line_pos] = '\0';
            editor_state.line_count++;
            line_pos = 0;
        } else if (line_pos < EDITOR_MAX_LINE_LENGTH - 1) {
            editor_state.lines[editor_state.line_count][line_pos] = buffer[i];
            line_pos++;
        }
    }
    
    if (editor_state.line_count == 0) {
        editor_state.line_count = 1;
        editor_state.lines[0][0] = '\0';
    }
    editor_state.modified = false;
    return true;
}

void editor_new_file(const char* filename) {
    strcpy(editor_state.filename, filename);
    editor_state.line_count = 1;
    editor_state.lines[0][0] = '\0';
    editor_state.modified = false;
}

bool editor_save_file(void) {
    
    uint32_t total_size = 0;
    for (int i = 0; i < editor_state.line_count; i++) {
        total_size += strlen(editor_state.lines[i]);
        if (i < editor_state.line_count - 1) {
            total_size++; 
        }
    }
    
    if (total_size >= sizeof(file_save_buffer)) {
        return false;  
    }
    
    char* buffer = file_save_buffer;
    int pos = 0;

    for (int i = 0; i < editor_state.line_count; i++) {
        int line_len = strlen(editor_state.lines[i]);
        memcpy(buffer + pos, editor_state.lines[i], line_len);
        pos += line_len;
        
        if (i < editor_state.line_count - 1) {
            buffer[pos] = '\n';
            pos++;
        }
    }

    buffer[pos] = '\0';
    bool success = fat16_write_file_content(editor_state.filename, buffer, total_size);
    
    if (success) {
        editor_state.modified = false;
    }
    
    return success;
}


void editor_refresh_screen(void) {
    vga_clear();
    editor_draw_text();
    editor_draw_status_line();
    editor_update_cursor_position();
}

void editor_draw_text(void) {
    for (int screen_row = 0; screen_row < EDITOR_SCREEN_HEIGHT; screen_row++) {
        int file_row = screen_row + editor_state.scroll_offset_y;

        if (file_row >= editor_state.line_count) {
            
            vga_putchar_at('~', 0, screen_row);
        } else {
            
            const char* line = editor_state.lines[file_row];
            int line_len = strlen(line);
            
            for (int screen_col = 0; screen_col < EDITOR_SCREEN_WIDTH; screen_col++) {
                int file_col = screen_col + editor_state.scroll_offset_x;
                
                if (file_col < line_len) {
                    vga_putchar_at(line[file_col], screen_col, screen_row);
                } else {
                    vga_putchar_at(' ', screen_col, screen_row);
                }
            }
        }
    }
}

void editor_draw_status_line(void) {
    
    for (int i = 0; i < EDITOR_SCREEN_WIDTH; i++) {
        vga_putchar_at(' ', i, EDITOR_STATUS_LINE);
    }

    const char* mode_str = "";
    switch (editor_state.mode) {
        case EDITOR_MODE_COMMAND:
            mode_str = "";
            break;
        case EDITOR_MODE_INSERT:
            mode_str = "-- INSERT --";
            break;
        case EDITOR_MODE_COMMAND_LINE:
            mode_str = ":";
            break;
    }
    
    vga_print_at(mode_str, 0, EDITOR_STATUS_LINE);
    char info[80];

    if (editor_state.mode == EDITOR_MODE_COMMAND_LINE) {
        strcpy(info, ":");
        strcat(info, editor_state.command_buffer);
    } else {
        
        strcpy(info, editor_state.filename);
        if (editor_state.modified) {
            strcat(info, " [+]");
        }
        char pos_info[32];
        strcat(info, " ");
        int_to_string(editor_state.cursor_y + 1, pos_info, 10);
        strcat(info, pos_info);
        strcat(info, ",");
        int_to_string(editor_state.cursor_x + 1, pos_info, 10);
        strcat(info, pos_info);
    }
    
    vga_print_at(info, 0, EDITOR_STATUS_LINE);
}

void editor_update_cursor_position(void) {
    editor_state.screen_y = editor_state.cursor_y - editor_state.scroll_offset_y;
    editor_state.screen_x = editor_state.cursor_x - editor_state.scroll_offset_x;
    
    vga_set_cursor(editor_state.screen_x, editor_state.screen_y);
}

void editor_scroll_if_needed(void) {
    
    if (editor_state.cursor_y < editor_state.scroll_offset_y) {
        editor_state.scroll_offset_y = editor_state.cursor_y;
    }
    if (editor_state.cursor_y >= editor_state.scroll_offset_y + EDITOR_SCREEN_HEIGHT) {
        editor_state.scroll_offset_y = editor_state.cursor_y - EDITOR_SCREEN_HEIGHT + 1;
    }
    
    
    if (editor_state.cursor_x < editor_state.scroll_offset_x) {
        editor_state.scroll_offset_x = editor_state.cursor_x;
    }
    if (editor_state.cursor_x >= editor_state.scroll_offset_x + EDITOR_SCREEN_WIDTH) {
        editor_state.scroll_offset_x = editor_state.cursor_x - EDITOR_SCREEN_WIDTH + 1;
    }
}


void editor_move_to_line_start(void) {
    editor_state.cursor_x = 0;
}

void editor_move_to_line_end(void) {
    editor_state.cursor_x = strlen(editor_state.lines[editor_state.cursor_y]);
}


void editor_insert_char(char c) {
    char* line = editor_state.lines[editor_state.cursor_y];
    int line_len = strlen(line);
    
    if (line_len < EDITOR_MAX_LINE_LENGTH - 1) {
        
        for (int i = line_len; i >= editor_state.cursor_x; i--) {
            line[i + 1] = line[i];
        }
        
        line[editor_state.cursor_x] = c;
        editor_state.cursor_x++;
        editor_state.modified = true;
    }
}

void editor_delete_char(void) {
    char* line = editor_state.lines[editor_state.cursor_y];
    int line_len = strlen(line);
    
    if (editor_state.cursor_x < line_len) {
        
        for (int i = editor_state.cursor_x; i < line_len; i++) {
            line[i] = line[i + 1];
        }
        editor_state.modified = true;
    }
}

void editor_backspace(void) {
    if (editor_state.cursor_x > 0) {
        editor_state.cursor_x--;
        editor_delete_char();
    } else if (editor_state.cursor_y > 0) {
        
        int prev_line_len = strlen(editor_state.lines[editor_state.cursor_y - 1]);
        if (prev_line_len + strlen(editor_state.lines[editor_state.cursor_y]) < EDITOR_MAX_LINE_LENGTH - 1) {
            strcat(editor_state.lines[editor_state.cursor_y - 1], editor_state.lines[editor_state.cursor_y]);
            
            for (int i = editor_state.cursor_y; i < editor_state.line_count - 1; i++) {
                strcpy(editor_state.lines[i], editor_state.lines[i + 1]);
            }
            
            editor_state.line_count--;
            editor_state.cursor_y--;
            editor_state.cursor_x = prev_line_len;
            editor_state.modified = true;
        }
    }
}

void editor_insert_newline(void) {
    if (editor_state.line_count < EDITOR_MAX_LINES - 1) {
        
        for (int i = editor_state.line_count; i > editor_state.cursor_y; i--) {
            strcpy(editor_state.lines[i], editor_state.lines[i - 1]);
        }
    
        char* current_line = editor_state.lines[editor_state.cursor_y];
        strcpy(editor_state.lines[editor_state.cursor_y + 1], current_line + editor_state.cursor_x);
        current_line[editor_state.cursor_x] = '\0';
        
        editor_state.line_count++;
        editor_state.cursor_y++;
        editor_state.cursor_x = 0;
        editor_state.modified = true;
    }
}

void editor_delete_line(void) {
    if (editor_state.line_count > 1) {
        
        for (int i = editor_state.cursor_y; i < editor_state.line_count - 1; i++) {
            strcpy(editor_state.lines[i], editor_state.lines[i + 1]);
        }
        editor_state.line_count--;
        
        if (editor_state.cursor_y >= editor_state.line_count) {
            editor_state.cursor_y = editor_state.line_count - 1;
        }
        
        editor_state.cursor_x = 0;
        editor_state.modified = true;
    } else {
        
        editor_state.lines[0][0] = '\0';
        editor_state.cursor_x = 0;
        editor_state.modified = true;
    }
}


void editor_process_command(const char* command) {
    if (strcmp(command, "w") == 0) {
        if (editor_save_file()) {
            editor_set_status_message("File saved");
        } else {
            editor_set_status_message("Error saving file");
        }
    } else if (strcmp(command, "q") == 0) {
        if (!editor_state.modified) {
            editor_state.should_exit = true;  
        } else {
            editor_set_status_message("File modified, use :q! to force quit");
        }
    } else if (strcmp(command, "q!") == 0) {
        editor_state.should_exit = true;  
    } else if (strcmp(command, "wq") == 0) {
        if (editor_save_file()) {
            editor_state.should_exit = true;  
        } else {
            editor_set_status_message("Error saving file");
        }
    } else {
        editor_set_status_message("Unknown command");
    }
}

void editor_set_status_message(const char* message) {
    strcpy(editor_state.status_message, message);
}