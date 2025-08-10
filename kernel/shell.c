#include "shell.h"
#include "vga.h"
#include "kernel.h"
#include "../filesystem/fat12.h"

/* Defintions */
#define MAX_INPUT 128

/* Global Variables */
static char input_buffer[MAX_INPUT];
static int input_pos = 0;

/* Function Declarations */
static int str_len(const char* str);
static int str_compare(const char* a, const char* b);
static void print_int(int num);


static void print_int(int num) {
    char buffer[16];
    int i = 0;
    
    if (num == 0) {
        print("0");
        return;
    }
    
    if (num < 0) {
        print("-");
        num = -num;
    }
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (--i >= 0) {
        enter_char(buffer[i]);
    }
}

static int str_len(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

static int str_compare(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

void shell_print_prompt() {
    print("@AcornOS$~: ");
    mark_inp_start(); 
}

void help_cmd() {
    println("\nAvailable commands:");
    println("help     - Display available commands");
    println("clear    - Clear screen");
    println("fstest   - Test for file system");
    println("ls       - List files in system");
}

void clear_cmd() {
    clr_scr();
    shell_print_prompt();
}

void fs_test_cmd() {
    println("\n FS test");
    
    if (fat12_is_initialized()) {
        println("File system initialized");
    } else {
        println("File system not initialized");
        return;
    }
    
    print("Testing directory listing... ");
    struct fat12_dir_entry entries[10];
    int count = fat12_list_directory(entries, 10);
    if (count >= 0) {
        println("Success");
        print("Found ");
        print_int(count);
        println(" entries");
    } else {
        println("Failed");
    }
    
    print("Testing file read... ");
    char buffer[64];
    int bytes_read = fat12_read_file("test.txt", buffer, 63);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        println("Success");
        print("Content: ");
        println(buffer);
    } else {
        println("Failed or file not found");
    }
    
    print("Testing file creation... ");
    if (fat12_create_file("newfile.txt", 0x20) == 0) {
        println("Success");
    } else {
        println("Failed");
    }
    
    print("Testing file write... ");
    const char* test_content = "THIS WORKS BAHHHHHHHHHHH";
    int bytes_written = fat12_write_file("write_test.txt", (void*)test_content, str_len(test_content));
    if (bytes_written > 0) {
        println("Success");
        print("Wrote ");
        print_int(bytes_written);
        println(" bytes");
    } else {
        println("Failed");
    }
    
    print("Testing written file read... ");
    char read_buffer[128];
    int read_bytes = fat12_read_file("write_test.txt", read_buffer, 127);
    if (read_bytes > 0) {
        read_buffer[read_bytes] = '\0';
        println("Success");
        print("Content: ");
        println(read_buffer);
    } else {
        println("Failed");
    }
    
    println("Test Complete");
}

void ls_cmd() {
    struct fat12_dir_entry entries[32];
    int count = fat12_list_directory(entries, 32);
    
    if (count < 0) {
        println("\nError reading directory");
        return;
    }
    
    if (count == 0) {
        println("\nDirectory is empty");
        return;
    }
    
    println("\nFiles:");
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < 8 && entries[i].filename[j] != ' '; j++) {
            enter_char(entries[i].filename[j]);
        }
        
        if (entries[i].extension[0] != ' ') {
            enter_char('.');
            for (int j = 0; j < 3 && entries[i].extension[j] != ' '; j++) {
                enter_char(entries[i].extension[j]);
            }
        }
        
        print(" (");
        print_int(entries[i].file_size);
        println(" bytes)");
    }
}

void execute_command(char* input) {
    if (input == NULL || input[0] == '\0') {
        shell_print_prompt();
        return;
    }
    
    while (*input == ' ') {
        input++;
    }

    if (*input == '\0') {
        shell_print_prompt();
        return;
    }
    if (str_compare(input, "help") == 0) {
        help_cmd();
    } 
    else if (str_compare(input, "clear") == 0) {
        clear_cmd();
        return;
    }
    else if (str_compare(input, "fstest") == 0) {
        fs_test_cmd();
    }
    else if (str_compare(input, "ls") == 0) {
        ls_cmd();
    }
    else {
        println("\nUnknown command");
    }
    shell_print_prompt();
}

void shell_process_char(char c) {
    enter_char(c);
    if (c == '\n' || c == '\r') {
        input_buffer[input_pos] = '\0';
        execute_command(input_buffer);
        input_pos = 0;
    } 
    else if (c == '\b') {
        if (input_pos > 0) {
            input_pos--;
        }
    }
    else if (input_pos < MAX_INPUT - 1) {
        input_buffer[input_pos++] = c;
    }
}

void shell_init() {
    input_pos = 0;
    clr_scr();
    println("AcornOS v0.1 - Type 'help' for commands");
    shell_print_prompt();
}