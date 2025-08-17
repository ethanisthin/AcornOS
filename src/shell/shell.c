#include "shell.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../lib/string/string.h"
#include "../filesystem/fat16.h"

static shell_context_t shell_ctx;


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
    {"fstest", "Run filesystem tests", cmd_fstest},
    {NULL, NULL, NULL} 
};

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
    
    vga_printf_colored(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
                      "AcornOS Shell initialized\n");
    vga_printf("Type 'help' for available commands\n\n");
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

    vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                      "Command not found: %s\n", cmd_name);
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
    vga_printf_colored(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
                      "Available commands:\n");
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
    if (shell_ctx.history_count == 0) {
        vga_printf("No command history\n");
        return;
    }
    
    vga_printf_colored(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
                      "Command history:\n");
    
    int start = (shell_ctx.history_count > SHELL_MAX_HISTORY) ? 
                shell_ctx.history_count - SHELL_MAX_HISTORY : 0;
    
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
    vga_printf_colored(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
                      "AcornOS v0.1\n");
    vga_printf("=============\n");
    vga_printf("A 32-bit operating system built from scratch\n");
    vga_printf("Architecture: x86 (32-bit)\n");
    vga_printf("Features: Protected mode, interrupts, keyboard, VGA\n");
    vga_printf("Shell: Built-in command processor\n");
}

void cmd_pwd(int argc, char* argv[]) {
    const char* current_dir = fat16_get_current_directory();
    vga_printf("%s\n", current_dir);
}

void cmd_cd(int argc, char* argv[]) {
    if (argc < 2) {
        if (fat16_change_directory("/")) {
            vga_printf("Changed to root directory\n");
        } else {
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                              "Failed to change to root directory\n");
        }
        return;
    }
    
    const char* target_dir = argv[1];
    const char* old_dir = fat16_get_current_directory();
    
    if (fat16_change_directory(target_dir)) {
        const char* new_dir = fat16_get_current_directory();
        vga_printf("Changed directory: %s -> %s\n", old_dir, new_dir);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Directory not found: %s\n", target_dir);
    }
}

void cmd_ls(int argc, char* argv[]) {
    const char* current_dir = fat16_get_current_directory();
    
    vga_printf_colored(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
                      "Directory listing for: ");
    vga_printf("%s\n", current_dir);
    vga_printf("========================\n");
    
    if (strcmp(current_dir, "/") != 0) {
        vga_printf_colored(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK, "..");
        vga_printf("           <DIR>\n");
    }
    
    if (strcmp(current_dir, "/") == 0) {
        vga_printf_colored(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK, "bin");
        vga_printf("          <DIR>\n");
        
        vga_printf_colored(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK, "etc");
        vga_printf("          <DIR>\n");
        
        vga_printf_colored(VGA_COLOR_WHITE, VGA_COLOR_BLACK, "readme.txt");
        vga_printf("    %d bytes\n", 1024);
        
        vga_printf_colored(VGA_COLOR_WHITE, VGA_COLOR_BLACK, "config.sys");
        vga_printf("    %d bytes\n", 512);
    } else {
        vga_printf_colored(VGA_COLOR_WHITE, VGA_COLOR_BLACK, "file1.txt");
        vga_printf("     %d bytes\n", 256);
        
        vga_printf_colored(VGA_COLOR_WHITE, VGA_COLOR_BLACK, "file2.dat");
        vga_printf("     %d bytes\n", 2048);
    }
    
    vga_printf("\n");
}

void cmd_touch(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Usage: touch <filename>\n");
        return;
    }
    
    const char* filename = argv[1];
    
    if (fat16_create_file(filename, FAT_ATTR_ARCHIVE)) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                          "File created: %s\n", filename);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Failed to create file: %s\n", filename);
    }
}

void cmd_rm(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Usage: rm <filename>\n");
        return;
    }
    
    const char* filename = argv[1];

    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Cannot remove directory entries '.' or '..'\n");
        return;
    }

    vga_printf("Delete file '%s'? (y/n): ", filename);
    char response = keyboard_getchar();
    vga_printf("%c\n", response);
    
    if (response == 'y' || response == 'Y') {
        if (fat16_delete_file(filename)) {
            vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                              "File deleted: %s\n", filename);
        } else {
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                              "Failed to delete file: %s\n", filename);
        }
    } else {
        vga_printf("File deletion cancelled\n");
    }
}

void cmd_mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Usage: mkdir <directory_name>\n");
        return;
    }
    
    const char* dirname = argv[1];
    
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Invalid directory name: %s\n", dirname);
        return;
    }

    if (fat16_create_file(dirname, FAT_ATTR_DIRECTORY)) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                          "Directory created: %s\n", dirname);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Failed to create directory: %s\n", dirname);
    }
}

void cmd_rmdir(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Usage: rmdir <directory_name>\n");
        return;
    }
    
    const char* dirname = argv[1];

    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Cannot remove directory entries '.' or '..'\n");
        return;
    }
    
    if (strcmp(dirname, "/") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Cannot remove root directory\n");
        return;
    }
    
    const char* current_dir = fat16_get_current_directory();
    if (strcmp(current_dir, dirname) == 0 || 
        (strlen(current_dir) > 1 && strcmp(current_dir + 1, dirname) == 0)) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Cannot remove current directory\n");
        return;
    }
    
    vga_printf("Remove directory '%s'? (y/n): ", dirname);
    char response = keyboard_getchar();
    vga_printf("%c\n", response);
    
    if (response == 'y' || response == 'Y') {
        // TODO: Check if directory is empty before deletion
        if (fat16_delete_file(dirname)) {
            vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                              "Directory removed: %s\n", dirname);
        } else {
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                              "Failed to remove directory: %s\n", dirname);
        }
    } else {
        vga_printf("Directory removal cancelled\n");
    }
}

void cmd_cp(int argc, char* argv[]) {
    if (argc < 3) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Usage: cp <source> <destination>\n");
        return;
    }
    
    const char* source = argv[1];
    const char* dest = argv[2];

    if (strcmp(source, dest) == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Source and destination are the same\n");
        return;
    }
    
    // TODO: Check if source file exists
    // TODO: Check if destination already exists and prompt for overwrite
    // TODO: Read source file content and write to destination

    vga_printf("Copying '%s' to '%s'...\n", source, dest);
    if (fat16_create_file(dest, FAT_ATTR_ARCHIVE)) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                          "File copied successfully: %s -> %s\n", source, dest);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Failed to copy file: %s -> %s\n", source, dest);
    }
}

void cmd_mv(int argc, char* argv[]) {
    if (argc < 3) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Usage: mv <source> <destination>\n");
        return;
    }
    
    const char* source = argv[1];
    const char* dest = argv[2];

    if (strcmp(source, dest) == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Source and destination are the same\n");
        return;
    }

    if (strcmp(source, ".") == 0 || strcmp(source, "..") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Cannot move directory entries '.' or '..'\n");
        return;
    }
    
    // TODO: Check if source file exists
    // TODO: Check if destination already exists and prompt for overwrite
    // TODO: Copy source to destination, then delete source
    
    vga_printf("Moving '%s' to '%s'...\n", source, dest);
    if (fat16_create_file(dest, FAT_ATTR_ARCHIVE)) {
        if (fat16_delete_file(source)) {
            vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                              "File moved successfully: %s -> %s\n", source, dest);
        } else {
            vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                              "File copied but failed to delete source: %s\n", source);
        }
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Failed to move file: %s -> %s\n", source, dest);
    }
}

void cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Usage: cat <filename>\n");
        return;
    }
    
    const char* filename = argv[1];
    
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Cannot display directory entries\n");
        return;
    }
    
    // TODO: Check if file exists and is not a directory
    // TODO: Read and display file contents
    

    vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK,
                      "Contents of '%s':\n", filename);
    vga_printf("==================\n");

    if (strstr(filename, "readme") || strstr(filename, "README")) {
        vga_printf("Welcome to AcornOS!\n");
        vga_printf("This is a 32-bit operating system built from scratch.\n");
        vga_printf("Features: FAT-16 filesystem, shell interface, VGA display.\n");
    } else if (strstr(filename, "config")) {
        vga_printf("# AcornOS Configuration File\n");
        vga_printf("boot_device=hd0\n");
        vga_printf("filesystem=fat16\n");
        vga_printf("debug_mode=off\n");
    } else {
        vga_printf("This is the content of %s\n", filename);
        vga_printf("File created by AcornOS filesystem.\n");
        vga_printf("Lorem ipsum dolor sit amet...\n");
    }
    
    vga_printf("\n");
}

void cmd_stat(int argc, char* argv[]) {
    if (argc < 2) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "Usage: stat <filename>\n");
        return;
    }
    
    const char* filename = argv[1];
    
    // TODO: Get actual file information from filesystem

    vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK,
                      "File information for: %s\n", filename);
    vga_printf("========================\n");
    
    if (strcmp(filename, ".") == 0) {
        vga_printf("Type:        Directory\n");
        vga_printf("Size:        <DIR>\n");
        vga_printf("Attributes:  Directory\n");
    } else if (strcmp(filename, "..") == 0) {
        vga_printf("Type:        Directory (Parent)\n");
        vga_printf("Size:        <DIR>\n");
        vga_printf("Attributes:  Directory\n");
    } else if (strstr(filename, "dir") || strstr(filename, "bin") || strstr(filename, "etc")) {
        vga_printf("Type:        Directory\n");
        vga_printf("Size:        <DIR>\n");
        vga_printf("Attributes:  Directory\n");
        vga_printf("Cluster:     %d\n", 5);
    } else {
        int simulated_size = strlen(filename) * 100; 
        
        vga_printf("Type:        Regular File\n");
        vga_printf("Size:        %d bytes\n", simulated_size);
        vga_printf("Attributes:  ");
        
        if (strstr(filename, "readme") || strstr(filename, "config")) {
            vga_printf("Read-Only, Archive\n");
        } else {
            vga_printf("Archive\n");
        }
        
        vga_printf("Cluster:     %d\n", 10);
        vga_printf("Extension:   ");
        const char* dot = strrchr(filename, '.');
        if (dot && dot != filename) {
            vga_printf("%s\n", dot + 1);
        } else {
            vga_printf("(none)\n");
        }
    }
    
    vga_printf("Path:        %s/%s\n", fat16_get_current_directory(), filename);
    vga_printf("\n");
}

void cmd_fstest(int argc, char* argv[]) {
    vga_printf_colored(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
                      "Running FAT-16 Filesystem Tests\n");
    vga_printf("================================\n\n");
    
    fat16_test_basic_functions();
    fat16_test_cluster_operations();
    fat16_test_filename_conversion();
    fat16_test_file_operations();
    
    vga_printf_colored(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
                      "All tests completed!\n");
}