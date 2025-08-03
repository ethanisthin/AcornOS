#include "shell.h"
#include "vga.h"
#include "kernel.h"
#include <stddef.h>

/* Definitions */
#define MAX_INPUT_LENGTH 256
#define MAX_ARGS 16
#define MAX_HISTORY 10

/* Global Variables*/
static char inp_buffer[MAX_INPUT_LENGTH];
static int inp_position = 0;
static char cmd_history[MAX_HISTORY][MAX_INPUT_LENGTH];
static int history_count = 0;
static int history_index = 0;

/* Struct Declaration */
struct cmd {
    const char* name;
    const char* cmd_description;
    void (*handler)(int argc, char* argv[]);
};

/* Function Declarations */
void help_cmd(int argc, char* argv[]);
void clear_cmd(int argc, char* argv[]);
void echo_cmd(int argc, char* argv[]);
void history_cmd(int argc, char* argv[]);
int str_len(const char* str);
int str_compare(const char* str1, const char* str2);
void str_copy(char* dest, const char* src);
int tokenize(char* input, char* argv[], int max_args);
void add_to_history(const char* command);
void execute_command(const char* input);
void shell_process_char(char c);
void shell_print_prompt();
void shell_init();


static struct cmd commands[] = {
    {"help", "Display available commands", help_cmd},
    {"clear", "Clear the screen", clear_cmd},
    {"echo", "Display text", echo_cmd},
    {"history", "Show command history", history_cmd},
    {NULL, NULL, NULL}
};

int str_len(const char* str){
    int len = 0;
    while (str[len]){
        len++;
    }
    return len;
}

int str_compare(const char* str1, const char* str2) {
    while (*str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

void str_copy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}


int tokenize(char* input, char* argv[], int max_args) {
    int argc = 0;
    char* current = input;
    
    while (*current && argc < max_args - 1) {
        while (*current == ' ' || *current == '\t') {
            current++;
        }
        if (*current == '\0') {
            break;
        }
        argv[argc++] = current;
        while (*current && *current != ' ' && *current != '\t') {
            current++;
        }
        if (*current) {
            *current++ = '\0';
        }
    }
    argv[argc] = NULL;
    return argc;
}

void help_cmd(int argc, char* argv[]) {
    println("Available commands:");
    for (int i = 0; commands[i].name; i++) {
        print("  ");
        print(commands[i].name);
        print(" - ");
        println(commands[i].cmd_description);
    }
}

void clear_cmd(int argc, char* argv[]) {
    clr_scr();
    shell_print_prompt();
    mark_inp_start();
}

void echo_cmd(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) print(" ");
        print(argv[i]);
    }
    enter_char('\n');
}

void history_cmd(int argc, char* argv[]) {
    println("Command history:");
    for (int i = 0; i < history_count; i++) {
        print("  ");
        println(cmd_history[i]);
    }
}

void add_to_history(const char* command) {
    if (str_len(command) == 0) return;
    
    if (history_count < MAX_HISTORY) {
        str_copy(cmd_history[history_count], command);
        history_count++;
    } else {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            str_copy(cmd_history[i], cmd_history[i + 1]);
        }
        str_copy(cmd_history[MAX_HISTORY - 1], command);
    }
    history_index = history_count;
}

void execute_command(const char* input) {
    if (str_len(input) == 0) {
        return;
    }

    char input_copy[MAX_INPUT_LENGTH];
    str_copy(input_copy, input);

    char* argv[MAX_ARGS];
    int argc = tokenize(input_copy, argv, MAX_ARGS);
    
    if (argc == 0) {
        return;
    }

    for (int i = 0; commands[i].name; i++) {
        if (str_compare(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }

    print("Unknown command: ");
    println(argv[0]);
}


void shell_process_char(char c) {
    if (c == '\n') {
        enter_char('\n');
        inp_buffer[inp_position] = '\0';
        
        if (inp_position > 0) {
            add_to_history(inp_buffer);
            execute_command(inp_buffer);
        }

        inp_position = 0;
        shell_print_prompt();
        mark_inp_start();
    } else if (c == '\b') {
        if (inp_position > 0) {
            inp_position--;
            enter_char(c);
        }
    } else if (c >= 32 && c <= 126) {
        if (inp_position < MAX_INPUT_LENGTH - 1) {
            inp_buffer[inp_position++] = c;
            enter_char(c);
        }
    }
}

void shell_print_prompt() {
    print("AcornOS> ");
}

void shell_init() {
    inp_position = 0;
    history_count = 0;
    history_index = 0;
    
    println("AcornOS Shell v1.0");
    println("Type 'help' for available commands");
    shell_print_prompt();
    mark_inp_start();
}