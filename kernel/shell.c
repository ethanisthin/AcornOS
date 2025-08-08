#include "shell.h"
#include "vga.h"
#include "kernel.h"
#include "../filesystem/fat12.h"

/* Defintions */
#define MAX_INPUT 128
#define CMD_HISTORY 5

/* Global Variables */
static char input_buffer[MAX_INPUT];
static int input_pos = 0;
static char history[CMD_HISTORY][MAX_INPUT];
static int history_pos = 0;
static int history_count = 0;

/* Function Declarations */
static int str_len(const char* str);
static int str_compare(const char* a, const char* b);
static int str_ncmp(const char* a, const char* b, int n);
static void str_copy(char* dest, const char* src);
static void print_int(int num);
static void add_history(const char* cmd);
static void show_history();
static void help_cmd();
static void clear_cmd();
static void echo_cmd(char* text);
static void ls_cmd();
static void fs_test_cmd();
static void cat_cmd(char* filename);
static void touch_cmd(char* filename);

static int str_len(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

static int str_compare(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

static int str_ncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (a[i] == '\0') return 0;
    }
    return 0;
}

static void str_copy(char* dest, const char* src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

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

static void add_history(const char* cmd) {
    if (!cmd || cmd[0] == '\0') return;
    
    str_copy(history[history_pos % CMD_HISTORY], cmd);
    history_pos++;
    if (history_count < CMD_HISTORY) history_count++;
}

static void show_history() {
    println("\nCommand history:");
    int start = history_pos > CMD_HISTORY ? history_pos % CMD_HISTORY : 0;
    
    for (int i = 0; i < history_count; i++) {
        int idx = (start + i) % CMD_HISTORY;
        print(" ");
        print_int(i + 1);
        print(". ");
        println(history[idx]);
    }
}

static void help_cmd() {
    println("\nAvailable commands:");
    println("help     - Show this help");
    println("clear    - Clear screen");
    println("echo     - Print text");
    println("history  - Show command history");
    println("ls       - List files");
    println("cat      - Display file contents");
    println("touch    - Create empty file");
    println("fstest   - Test file system");
}

static void clear_cmd() {
    clr_scr();
    shell_print_prompt();
}

static void echo_cmd(char* text) {
    print("\n");
    print(text);
}

static void ls_cmd() {
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

static void fs_test_cmd() {
    println("\n FAT12 test");
    
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
    
    println("Test Complete");
}

static void cat_cmd(char* filename) {
    if (!filename || filename[0] == '\0') {
        println("\nUsage: cat <filename>");
        return;
    }
    
    char buffer[512];
    int bytes_read = fat12_read_file(filename, buffer, 511);
    
    if (bytes_read < 0) {
        print("\nFile not found: ");
        println(filename);
        return;
    }
    
    if (bytes_read == 0) {
        println("\nFile is empty");
        return;
    }
    
    buffer[bytes_read] = '\0';
    println("");
    print(buffer);
}

static void touch_cmd(char* filename) {
    if (!filename || filename[0] == '\0') {
        println("\nUsage: touch <filename>");
        return;
    }
    
    if (fat12_create_file(filename, 0x20) == 0) {
        print("\nCreated file: ");
        println(filename);
    } else {
        print("\nFailed to create file: ");
        println(filename);
    }
}

void execute_command(char* input) {
    if (str_len(input) == 0) return;
    
    add_history(input);
    
    if (str_compare(input, "help") == 0) {
        help_cmd();
    } 
    else if (str_compare(input, "clear") == 0) {
        clear_cmd();
        return;
    }
    else if (str_ncmp(input, "echo ", 5) == 0) {
        echo_cmd(input + 5);
    }
    else if (str_compare(input, "history") == 0) {
        show_history();
    }
    else if (str_compare(input, "ls") == 0) {
        ls_cmd();
    }
    else if (str_compare(input, "fstest") == 0) {
        fs_test_cmd();
    }
    else if (str_ncmp(input, "cat ", 4) == 0) {
        cat_cmd(input + 4);
    }
    else if (str_ncmp(input, "touch ", 6) == 0) {
        touch_cmd(input + 6);
    }
    else {
        println("");
        print("Unknown command: ");
        print(input);
        println("");
        return;
    }
    shell_print_prompt();
}

void shell_process_char(char c) {
    if (c == '\n') {
        input_buffer[input_pos] = '\0';
        execute_command(input_buffer);
        input_pos = 0;
    } 
    else if (c == '\b') {
        if (input_pos > 0) {
            input_pos--;
            enter_char(c);
            update_cursor();
        }
    }
    else if (input_pos < MAX_INPUT - 1) {
        input_buffer[input_pos++] = c;
        enter_char(c);
        update_cursor();
    }
}

void shell_print_prompt() {
    print("\n@AcornOS:~$ ");
    mark_inp_start();
    update_cursor();
}

void shell_init() {
    input_pos = 0;
    history_pos = 0;
    history_count = 0;
    clr_scr();
    print("AcornOS v0.1 - Type 'help' for commands\n");
    shell_print_prompt();
}