#include "shell.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../lib/string/string.h"

static shell_context_t shell_ctx;


static const shell_command_t commands[] = {
    {"help", "Show available commands", cmd_help},
    {"clear", "Clear the screen", cmd_clear},
    {"echo", "Display text", cmd_echo},
    {"history", "Show command history", cmd_history},
    {"about", "Show system information", cmd_about},
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
        vga_printf("%3d: %s\n", i + 1, shell_ctx.history[index]);
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