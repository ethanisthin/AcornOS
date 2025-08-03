#include "shell.h"
#include "vga.h"
#include "kernel.h"

#define MAX_INPUT 128
static char input_buffer[MAX_INPUT];
static int input_pos = 0;

// Basic commands
void help_cmd() {
    println("\nAvailable commands:");
    println("help    - Show this help");
    println("clear   - Clear screen");
    println("echo    - Print text");
}

void clear_cmd() {
    clr_scr();
    shell_print_prompt();
}

void echo_cmd(char* text) {
    println("");
    println(text);
}

// Command execution
void execute_command(char* input) {
    if (str_len(input) == 0) return;

    if (str_compare(input, "help") == 0) {
        help_cmd();
    } 
    else if (str_compare(input, "clear") == 0) {
        clear_cmd();
    }
    else if (str_ncmp(input, "echo ", 5) == 0) {
        echo_cmd(input + 5);
    }
    else {
        println("");
        print("Unknown command: ");
        println(input);
    }
}

// String utilities
int str_len(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int str_compare(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

int str_ncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (a[i] == '\0') return 0;
    }
    return 0;
}

// Input processing
void shell_process_char(char c) {
    if (c == '\n') {
        enter_char('\n');
        input_buffer[input_pos] = '\0';
        execute_command(input_buffer);
        input_pos = 0;
        shell_print_prompt();
    } 
    else if (c == '\b') {
        if (input_pos > 0) {
            input_pos--;
            enter_char(c);
        }
    }
    else if (input_pos < MAX_INPUT - 1) {
        input_buffer[input_pos++] = c;
        enter_char(c);
    }
}

void shell_print_prompt() {
    print("> \n");
}

void shell_init() {
    input_pos = 0;
    clr_scr();
    println("AcornOS v1.0 - Type 'help' for commands");
    shell_print_prompt();
}