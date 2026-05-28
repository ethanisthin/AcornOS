/**
    shell.c

    - includes all implementation of the shell functionality in AcornOS
*/

#include "shell.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../lib/string/string.h"
#include "../filesystem/fat16.h"
#include "../vim_editor/editor.h"
#include "../debug_params.h"

static shell_context_t shell_ctx;

/*
    shell_command_t

    - all the supported commands by the shell, will be updated if there is anything to add
*/
static const shell_command_t commands[] = {
    {"help", "Show available commands", cmd_help},
    {"clear", "Clear the screen", cmd_clear},
    {"echo", "Display text", cmd_echo},
    {"history", "Show command history", cmd_history},
    {"about", "Show system information", cmd_about},
    {"pwd", "Show current directory", cmd_pwd},        
    {"cd", "Change directory", cmd_cd},                
    {"ls", "List directory contents", cmd_ls}, 
    {"touch", "Create empty file", cmd_touch},        
    {"rm", "Remove file", cmd_rm}, 
    {"mkdir", "Create directory", cmd_mkdir},        
    {"rmdir", "Remove directory", cmd_rmdir},  
    {"cp", "Copy file", cmd_cp},                     
    {"mv", "Move/rename file", cmd_mv}, 
    {"cat", "Display file contents", cmd_cat},
    {"stat", "Show file information", cmd_stat}, 
    // {"fstest", "Run filesystem tests", cmd_fstest},
    {"format", "Format disk with FAT-16", cmd_format},
    {"mount", "Check/mount filesystem", cmd_mount},
    {"write", "Write text to file (write \"text\" > filename)", cmd_echo_to_file},
    {"edit", "Open text editor", cmd_edit},
    {NULL, NULL, NULL} 
};

/*
    shell_init()

    - init function for all shell functions (old and new)

*/
void shell_init(void) {
    shell_ctx.argc = 0;
    shell_ctx.history_count = 0;
    shell_ctx.history_index = 0;
    
    memset(shell_ctx.input_buffer, 0, SHELL_MAX_INPUT);
    for (int i = 0; i < SHELL_MAX_ARGS; i++) {
        shell_ctx.args[i] = NULL;
    }
    
    for (int i = 0; i < SHELL_MAX_HISTORY; i++) {
        memset(shell_ctx.history[i], 0, SHELL_MAX_INPUT);
    }

    vga_printf_colored(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "\n"
        "\xB0\xDB\xDB\xDB\xDB\xDB\xBF\xB0\xB0\xDB\xDB\xDB\xDB\xDB\xBF\xB0\xB0\xDB\xDB\xDB\xDB\xDB\xBF\xDB\xDB\xDB\xDB\xDB\xDB\xBF\xB0\xDB\xDB\xDB\xBF\xB0\xB0\xDB\xDB\xBF\xB0\xDB\xDB\xDB\xDB\xDB\xBF\xB0\xB0\xDB\xDB\xDB\xDB\xDB\xDB\xBF\n"
        "\xDB\xDB\xC9\xCD\xCD\xDB\xDB\xBF\xDB\xDB\xC9\xCD\xCD\xDB\xDB\xBF\xDB\xDB\xC9\xCD\xCD\xDB\xDB\xBF\xDB\xDB\xC9\xCD\xCD\xDB\xDB\xBF\xDB\xDB\xDB\xDB\xBF\xB0\xDB\xDB\xBA\xDB\xDB\xC9\xCD\xCD\xDB\xDB\xBF\xDB\xDB\xC9\xCD\xCD\xCD\xCD\xBC\n"
        "\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xBA\xDB\xDB\xBA\xB0\xB0\xC8\xCD\xBC\xDB\xDB\xBA\xB0\xB0\xDB\xDB\xBA\xDB\xDB\xDB\xDB\xDB\xDB\xC9\xBC\xDB\xDB\xC9\xDB\xDB\xBF\xDB\xDB\xBA\xDB\xDB\xBA\xB0\xB0\xDB\xDB\xBA\xC8\xDB\xDB\xDB\xDB\xDB\xBF\xB0\n"
        "\xDB\xDB\xC9\xCD\xCD\xDB\xDB\xBA\xDB\xDB\xBA\xB0\xB0\xDB\xDB\xBF\xDB\xDB\xBA\xB0\xB0\xDB\xDB\xBA\xDB\xDB\xC9\xCD\xCD\xDB\xDB\xBF\xDB\xDB\xBA\xC8\xDB\xDB\xDB\xDB\xBA\xDB\xDB\xBA\xB0\xB0\xDB\xDB\xBA\xB0\xC8\xCD\xCD\xCD\xDB\xDB\xBF\n"
        "\xDB\xDB\xBA\xB0\xB0\xDB\xDB\xBA\xC8\xDB\xDB\xDB\xDB\xDB\xC9\xBC\xC8\xDB\xDB\xDB\xDB\xDB\xC9\xBC\xDB\xDB\xBA\xB0\xB0\xDB\xDB\xBA\xDB\xDB\xBA\xB0\xDB\xDB\xDB\xBA\xC8\xDB\xDB\xDB\xDB\xDB\xC9\xBC\xDB\xDB\xDB\xDB\xDB\xDB\xC9\xBC\n"
        "\xC8\xCD\xBC\xB0\xB0\xC8\xCD\xBC\xB0\xC8\xCD\xCD\xCD\xCD\xBC\xB0\xB0\xC8\xCD\xCD\xCD\xCD\xBC\xB0\xC8\xCD\xBC\xB0\xB0\xC8\xCD\xBC\xC8\xCD\xBC\xB0\xB0\xC8\xCD\xCD\xCD\xBC\xB0\xC8\xCD\xCD\xCD\xCD\xBC\xB0\xC8\xCD\xCD\xCD\xCD\xCD\xBC\xB0\n");
    vga_printf("Type 'help' for available commands\n\n");

    keyboard_tab_handle(shell_tab_handle);
    keyboard_history_handle(shell_history_handle);
}

/*
    shell_history_handle()

    - a function that implements command history using arrow keys

*/
void shell_history_handle(char* buffer, uint32_t* pos, uint32_t max_length, int direction){
    static bool browsing = false;
    static char saved_input[SHELL_MAX_INPUT];
    static int browse_idx = 0;

    if (shell_ctx.history_count == 0){
        return;
    }

    if (!browsing) {
        if (direction != -1) return; 
        strncpy(saved_input, buffer, SHELL_MAX_INPUT - 1);
        saved_input[SHELL_MAX_INPUT - 1] = '\0';
        browsing = true;
        browse_idx = shell_ctx.history_count - 1;
    } else {
        browse_idx += direction; 
        if (browse_idx < 0) {
            browse_idx = 0;
            return;
        }
        if (browse_idx >= shell_ctx.history_count) {
            browsing = false;
            strncpy(buffer, saved_input, max_length - 1);
            buffer[max_length - 1] = '\0';
            *pos = strlen(buffer);
            for (uint32_t i = 0; i < *pos; i++) {
                vga_printf("\b \b");
            }
            *pos = strlen(saved_input);
            vga_printf("%s", saved_input);
            return;
        }
    }

    int idx = browse_idx % SHELL_MAX_HISTORY;
    const char* entry = shell_ctx.history[idx];

    for (uint32_t i = 0; i < *pos; i++) {
        vga_printf("\b \b");
    }
    
    strncpy(buffer, entry, max_length - 1);
    buffer[max_length - 1] = '\0';
    *pos = strlen(entry);
    vga_printf("%s", entry);

}


void shell_run(void) {
    while (1) {
        shell_print_prompt();
        keyboard_gets(shell_ctx.input_buffer, SHELL_MAX_INPUT);
        if (strlen(shell_ctx.input_buffer) == 0) {
            continue;
        }
    
        shell_add_to_history(&shell_ctx, shell_ctx.input_buffer);
        shell_parse_input(shell_ctx.input_buffer, &shell_ctx);
        shell_execute_command(&shell_ctx);
        
        vga_printf("\n");
    }
}

void shell_print_prompt(void) {
    vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK, SHELL_PROMPT);
}


void shell_tab_handle(char *buffer, uint32_t *pos, uint32_t max_length){
    int word_start = *pos;
    while (word_start > 0 && !is_space(buffer[word_start-1])){
        word_start--;
    }

    int word_idx = 0;
    int word_in = 0;

    for (int i=0; i<word_start; i++){
        if (!is_space(buffer[i]) && !word_in){
            word_idx++;
            word_in = 1;
        } else if (is_space(buffer[i])) {
            word_in = 0;
        }
    }
    
    int word_len = *pos - word_start;
    char partial[64];
    if (word_len > 0 && word_len < 64) {
        strncpy(partial, buffer + word_start, word_len);
    }
    partial[word_len] = '\0';
    static char completion_matches[64][32];
    int match_count = 0;
    if (word_idx == 0) {
        for (int i = 0; commands[i].name != NULL && match_count < 64; i++) {
            if (strncmp(partial, commands[i].name, word_len) == 0) {
                strncpy(completion_matches[match_count], commands[i].name, 31);
                completion_matches[match_count][31] = '\0';
                match_count++;
            }
        }
    } else {
        static fat16_dir_entry_t entries[64];
        int entry_count;
        uint16_t cluster = fat16_get_current_dir_cluster();
        if (fat16_read_directory_cluster(cluster, entries, 64, &entry_count)) {
            for (int i = 0; i < entry_count && match_count < 64; i++) {
                char filename[13];
                fat16_83_to_filename(entries[i].filename, filename);
                if (strlen(filename) > 0 && strncmp(partial, filename, word_len) == 0) {
                    strncpy(completion_matches[match_count], filename, 31);
                    completion_matches[match_count][31] = '\0';
                    match_count++;
                }
            }
        }
    }
    if (match_count == 0) {
        return;
    }
    if (match_count == 1) {
        const char* suffix = completion_matches[0] + word_len;
        vga_printf("%s", suffix);
        strncpy(buffer + word_start, completion_matches[0], max_length - word_start - 1);
        *pos = word_start + strlen(completion_matches[0]);
        buffer[*pos] = '\0';
    } else {
        vga_printf("\n");
        for (int i = 0; i < match_count; i++) {
            if (i > 0) vga_printf("  ");
            vga_printf("%s", completion_matches[i]);
        }
        vga_printf("\n");
        shell_print_prompt();
        buffer[*pos] = '\0';
        vga_printf("%s", buffer);
    }
}

void shell_parse_input(const char* input, shell_context_t* ctx) {
    ctx->argc = 0;
    
    static char work_buffer[SHELL_MAX_INPUT];
    strncpy(work_buffer, input, SHELL_MAX_INPUT - 1);
    work_buffer[SHELL_MAX_INPUT - 1] = '\0';
    
    char* ptr = work_buffer;
    
    while (*ptr && is_space(*ptr)) {
        ptr++;
    }
    
    while (*ptr && ctx->argc < SHELL_MAX_ARGS - 1) {
        ctx->args[ctx->argc] = ptr;
        ctx->argc++;
        
        while (*ptr && !is_space(*ptr)) {
            ptr++;
        }
        
        if (*ptr) {
            *ptr = '\0';
            ptr++;
            while (*ptr && is_space(*ptr)) {
                ptr++;
            }
        }
    }
    
    ctx->args[ctx->argc] = NULL;
}

void shell_execute_command(shell_context_t* ctx) {
    if (ctx->argc == 0) {
        return;
    }
    
    const char* cmd_name = ctx->args[0];
    
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(cmd_name, commands[i].name) == 0) {
            commands[i].handler(ctx->argc, ctx->args);
            return;
        }
    }
    vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Command not found: %s\n", cmd_name);
    vga_printf("Type 'help' for available commands\n");
}

void shell_add_to_history(shell_context_t* ctx, const char* input) {
    if (strlen(input) == 0) {
        return;
    }
    
    if (ctx->history_count > 0 && 
        strcmp(ctx->history[(ctx->history_count - 1) % SHELL_MAX_HISTORY], input) == 0) {
        return;
    }
    
    int index = ctx->history_count % SHELL_MAX_HISTORY;
    strncpy(ctx->history[index], input, SHELL_MAX_INPUT - 1);
    ctx->history[index][SHELL_MAX_INPUT - 1] = '\0';
    
    ctx->history_count++;
}

void cmd_help(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    vga_printf_colored(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,"Available commands:\n");
    vga_printf("==================\n");
    
    for (int i = 0; commands[i].name != NULL; i++) {
        vga_printf_colored(VGA_COLOR_WHITE, VGA_COLOR_BLACK, "%s", commands[i].name);
        
        int name_len = strlen(commands[i].name);
        for (int j = name_len; j < 10; j++) {
            vga_printf(" ");
        }
        vga_printf(" - %s\n", commands[i].description);
    }
}

void cmd_clear(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    vga_clear();
}

void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        vga_printf("%s", argv[i]);
        if (i < argc - 1) {
            vga_printf(" ");
        }
    }
    vga_printf("\n");
}

void cmd_history(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    if (shell_ctx.history_count == 0) {
        vga_printf("No command history\n");
        return;
    }
    vga_printf_colored(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,"Command history:\n");
    
    int start = (shell_ctx.history_count > SHELL_MAX_HISTORY) ? shell_ctx.history_count - SHELL_MAX_HISTORY : 0;
    
    for (int i = start; i < shell_ctx.history_count; i++) {
        int index = i % SHELL_MAX_HISTORY;
        int num = i + 1;
        if (num < 10) {
            vga_printf("  %d: %s\n", num, shell_ctx.history[index]);
        } else if (num < 100) {
            vga_printf(" %d: %s\n", num, shell_ctx.history[index]);
        } else {
            vga_printf("%d: %s\n", num, shell_ctx.history[index]);
        }
    }
}

void cmd_about(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    vga_printf_colored(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,"AcornOS v0.1\n");
    vga_printf("=============\n");
    vga_printf("A 32-bit operating system built from scratch\n");
    vga_printf("Architecture: x86 (32-bit)\n");
    vga_printf("Features: Protected mode, interrupts, keyboard, VGA\n");
    vga_printf("Shell: Built-in command processor\n");
}

void cmd_pwd(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    const char* current_dir = fat16_get_current_directory();
    vga_printf("%s\n", current_dir);
}

void cmd_cd(int argc, char* argv[]) {
    if (argc < 2) {
        if (fat16_change_directory("/")) {
            vga_printf("Changed to root directory\n");
        } else {
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to change to root directory\n");
        }
        return;
    }
    
    const char* target_dir = argv[1];
    const char* old_dir = fat16_get_current_directory();
    
    if (fat16_change_directory(target_dir)) {
        const char* new_dir = fat16_get_current_directory();
        vga_printf("Changed directory: %s -> %s\n", old_dir, new_dir);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Directory not found: %s\n", target_dir);
    }
}

void cmd_ls(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    const char* current_dir = fat16_get_current_directory();
    
    vga_printf_colored(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,"Directory listing for: ");
    vga_printf("%s\n", current_dir);
    vga_printf("========================\n");
    
    if (!fs_ctx.mounted) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"Filesystem not mounted - showing simulated content\n");
        return;
    }
    
    static fat16_dir_entry_t entries[64]; 
    int entry_count;
    
    // Get current directory cluster from fat16 module
    uint16_t current_cluster = fat16_get_current_dir_cluster();
    
    // Use the new function to read current directory
    if (!fat16_read_directory_cluster(current_cluster, entries, 64, &entry_count)) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to read directory\n");
        return;
    }
    
    if (entry_count == 0) {
        vga_printf("Directory is empty\n");
        return;
    }
    
    for (int i = 0; i < entry_count; i++) {
        fat16_dir_entry_t* entry = &entries[i];
        char filename[13];
        fat16_83_to_filename(entry->filename, filename);
        
        if (entry->attributes & FAT_ATTR_DIRECTORY) {
            vga_printf_colored(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK, "%s", filename);
            vga_printf("          <DIR>\n");
        } else {
            vga_printf_colored(VGA_COLOR_WHITE, VGA_COLOR_BLACK, "%s", filename);
            vga_printf("    %d bytes\n", entry->file_size);
        }
    }
    
    vga_printf("\nTotal: %d entries\n", entry_count);
}

void cmd_touch(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Usage: touch <filename>\n");
        return;
    }
    
    const char* filename = argv[1];
    if (fat16_create_file(filename, FAT_ATTR_ARCHIVE)) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"File created: %s\n", filename);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to create file: %s\n", filename);
    }
}

void cmd_rm(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Usage: rm <filename>\n");
        return;
    }
    
    const char* filename = argv[1];
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Cannot remove directory entries '.' or '..'\n");
        return;
    }

    vga_printf("Delete file '%s'? (y/n): ", filename);
    char response = keyboard_getchar();
    vga_printf("%c\n", response);
    
    if (response == 'y' || response == 'Y') {
        if (fat16_delete_file(filename)) {
            vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"File deleted: %s\n", filename);
        } else {
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to delete file: %s\n", filename);
        }
    } else {
        vga_printf("File deletion cancelled\n");
    }
}

void cmd_mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Usage: mkdir <directory_name>\n");
        return;
    }
    
    const char* dirname = argv[1];
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Invalid directory name: %s\n", dirname);
        return;
    }

    if (fat16_create_subdirectory(dirname)) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"Directory created: %s\n", dirname);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to create directory: %s\n", dirname);
    }
}

void cmd_rmdir(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Usage: rmdir <directory_name>\n");
        return;
    }
    
    const char* dirname = argv[1];
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Cannot remove directory entries '.' or '..'\n");
        return;
    }
    
    if (strcmp(dirname, "/") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Cannot remove root directory\n");
        return;
    }

    const char* current_dir = fat16_get_current_directory();
    if (strcmp(current_dir, dirname) == 0 || 
        (strlen(current_dir) > 1 && strcmp(current_dir + 1, dirname) == 0)) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Cannot remove current directory\n");
        return;
    }
    
    if (!fs_ctx.mounted) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Filesystem not mounted\n");
        return;
    }
    
    vga_printf("Remove directory '%s'? (y/n): ", dirname);
    char response = keyboard_getchar();
    vga_printf("%c\n", response);
    
    if (response == 'y' || response == 'Y') {
        if (fat16_delete_directory_entry(dirname)) {
            vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"Directory removed: %s\n", dirname);
        } else {
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to remove directory: %s\n", dirname);
        }
    } else {
        vga_printf("Directory removal cancelled\n");
    }
}

void cmd_cp(int argc, char* argv[]) {
    if (argc < 3) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Usage: cp <source> <destination>\n");
        return;
    }
    
    const char* source = argv[1];
    const char* dest = argv[2];
    if (strcmp(source, dest) == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Source and destination are the same\n");
        return;
    }
    
    if (!fs_ctx.mounted) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Filesystem not mounted\n");
        return;
    }
    
    static char file_buffer[2048];
    uint32_t bytes_read = 0;
    
    if (!fat16_read_file_content(source, file_buffer, sizeof(file_buffer), &bytes_read)) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to read source file: %s\n", source);
        return;
    }
    
    if (!fat16_create_file(dest, FAT_ATTR_ARCHIVE)) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to create destination file: %s\n", dest);
        return;
    }
    
    if (bytes_read > 0) {
        if (fat16_write_file_content_in_current_dir(dest, file_buffer, bytes_read)) {
            vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"File copied successfully: %s -> %s (%d bytes)\n",  source, dest, bytes_read);
        } else {
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to write to destination file\n");
        }
    } else {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"Empty file copied successfully: %s -> %s\n", source, dest);
    }
}

void cmd_mv(int argc, char* argv[]) {
    if (argc < 3) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Usage: mv <source> <destination>\n");
        return;
    }
    
    const char* source = argv[1];
    const char* dest = argv[2];
    
    
    if (strcmp(source, dest) == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Source and destination are the same\n");
        return;
    }
    
    if (strcmp(source, ".") == 0 || strcmp(source, "..") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Cannot move directory entries '.' or '..'\n");
        return;
    }
    
    if (!fs_ctx.mounted) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Filesystem not mounted\n");
        return;
    }
    
    static fat16_dir_entry_t entries[64];
    int entry_count;
    if (!fat16_read_directory_cluster(fat16_get_current_dir_cluster(), entries, 64, &entry_count)) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK, "Failed to read directory\n");
        return;
    }
    
    for (int i = 0; i < entry_count; i++) {
        char entry_filename[13];
        fat16_83_to_filename(entries[i].filename, entry_filename);
        if (strcmp(entry_filename, source) == 0) {  
            fat16_dir_entry_t new_entry;
            memcpy(&new_entry, &entries[i], sizeof(fat16_dir_entry_t));
            fat16_filename_to_83(dest, new_entry.filename);
            if (fat16_write_directory_entry_to_cluster(&new_entry, fat16_get_current_dir_cluster())) {
                if (fat16_delete_directory_entry(source)) {
                    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"File moved successfully: %s -> %s\n", source, dest);
                    vga_printf("Note: Only directory entry moved (file content not relocated)\n");
                } else {
                    vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK,"File copied but failed to delete source: %s\n", source);
                }
            } else {
                vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to create destination file\n");
            }
            return;
        }
    }
    vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Source file not found: %s\n", source);
}

void cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Usage: cat <filename>\n");
        return;
    }
    
    const char* filename = argv[1];
    static char file_buffer[4096];
    uint32_t bytes_read;
    if (fat16_read_file_content(filename, file_buffer, sizeof(file_buffer) - 1, &bytes_read)) {
        file_buffer[bytes_read] = '\0';
        
        // Show current directory in the path
        const char* current_dir = fat16_get_current_directory();
        vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK,
                          "Contents of %s%s%s:\n", 
                          current_dir,
                          (strcmp(current_dir, "/") == 0) ? "" : "/",
                          filename);
        vga_printf("%s\n", file_buffer);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to read file: %s\n", filename);
    }
}

void cmd_stat(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Usage: stat <filename>\n");
        return;
    }

    const char* filename = argv[1];
    if (!fs_ctx.mounted) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Filesystem not mounted\n");
        return;
    }

    static fat16_dir_entry_t entries[64];
    int entry_count;
    if (!fat16_read_directory_cluster(fat16_get_current_dir_cluster(), entries, 64, &entry_count)) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK, "Failed to read directory\n");
        return;
    }
    for (int i = 0; i < entry_count; i++) {
        fat16_dir_entry_t* entry = &entries[i];
        char entry_filename[13];
        fat16_83_to_filename(entry->filename, entry_filename);
    
        if (strcmp(entry_filename, filename) == 0) {
            vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK,"File information for: %s\n", filename);
            vga_printf("========================\n");
            
            if (entry->attributes & FAT_ATTR_DIRECTORY) {
                vga_printf("Type:        Directory\n");
                vga_printf("Size:        <DIR>\n");
            } else {
                vga_printf("Type:        Regular File\n");
                vga_printf("Size:        %d bytes\n", entry->file_size);
            }
            
            vga_printf("Attributes:  ");
            if (entry->attributes & FAT_ATTR_READ_ONLY) vga_printf("Read-Only ");
            if (entry->attributes & FAT_ATTR_HIDDEN) vga_printf("Hidden ");
            if (entry->attributes & FAT_ATTR_SYSTEM) vga_printf("System ");
            if (entry->attributes & FAT_ATTR_DIRECTORY) vga_printf("Directory ");
            if (entry->attributes & FAT_ATTR_ARCHIVE) vga_printf("Archive ");
            vga_printf("\n");
            
            vga_printf("First Cluster: %d\n", entry->first_cluster_low);
            const char* current_dir = fat16_get_current_directory();
            if (strcmp(current_dir, "/") == 0) {
                vga_printf("Path:        /%s\n", filename);
            } else {
                vga_printf("Path:        %s/%s\n", current_dir, filename);
            }
            vga_printf("\n");
            return;
        }
    }
    vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"File not found: %s\n", filename);
}


void cmd_format(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    vga_printf_colored(VGA_COLOR_BROWN, VGA_COLOR_BLACK, "WARNING: This will format the disk with FAT-16!\n");
    vga_printf("Continue? (y/n): ");
    char response = keyboard_getchar();
    vga_printf("%c\n", response);
    
    if (response == 'y' || response == 'Y') {
        vga_printf("Starting format operation...\n");
        if (fat16_format_disk()) {
            vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"Disk formatted successfully!\n");
            vga_printf("Returning to shell...\n");
        } else {
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Disk formatting failed!\n");
        }
        vga_printf("Format command completed\n");
    } else {
        vga_printf("Formatting cancelled\n");
    }
}

void cmd_mount(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    vga_printf("=== Mounting the File System ===\n");
    
    vga_printf("Checking filesystem status...\n");
    vga_printf("Mounted: %s\n", fs_ctx.mounted ? "YES" : "NO");
    
    if (!fs_ctx.mounted) {
        vga_printf("Filesystem not mounted, attempting to mount...\n");
        
        vga_printf("Calling fat16_mount()...\n");
        bool mount_result = fat16_mount();
        vga_printf("fat16_mount() returned: %s\n", mount_result ? "true" : "false");
        
        if (mount_result) {
            vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"Mount successful!\n");
        } else {
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Mount failed!\n");
        }
    } else if (FS_DEBUG) {
        vga_printf("Filesystem already mounted:\n");
        vga_printf("FAT starts at sector: %d\n", fs_ctx.fat_start_sector);
        vga_printf("Root directory at sector: %d\n", fs_ctx.root_dir_start_sector);
        vga_printf("Data area at sector: %d\n", fs_ctx.data_start_sector);
        vga_printf("Total clusters: %d\n", fs_ctx.total_clusters);
    }
    
    vga_printf("=== Mount Command Completed ===\n");
}

void cmd_echo_to_file(int argc, char* argv[]) {
    if (argc < 4) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Usage: write text > filename\n");
        vga_printf("Example: write hello > test.txt\n");
        vga_printf("Example: write hello world > test.txt\n");
        return;
    }
    
    int redirect_pos = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], ">") == 0) {
            redirect_pos = i;
            break;
        }
    }
    
    if (redirect_pos == -1 || redirect_pos == argc - 1) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Invalid syntax. Use: write text > filename\n");
        return;
    }
    
    const char* filename = argv[redirect_pos + 1];
    
    if (!fs_ctx.mounted) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Filesystem not mounted\n");
        return;
    }
    
    static char text_buffer[512];
    text_buffer[0] = '\0';
    
    for (int i = 1; i < redirect_pos; i++) {
        if (i > 1) {
            strcat(text_buffer, " "); 
        }
        strcat(text_buffer, argv[i]);
    }
    
    if (fat16_write_file_content_in_current_dir(filename, text_buffer, strlen(text_buffer))) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,"Text written to file: %s\n", filename);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,"Failed to write to file: %s\n", filename);
    }
}

void cmd_edit(int argc, char* argv[]) {
    if (argc < 2) {
        vga_puts("Usage: edit <filename>\n");
        return;
    }
    
    editor_run(argv[1]);
}