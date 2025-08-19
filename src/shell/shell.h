#ifndef SHELL_H
#define SHELL_H

#include "../include/kernel/types.h"

#define SHELL_MAX_INPUT 256
#define SHELL_MAX_ARGS 16
#define SHELL_MAX_HISTORY 10
#define SHELL_PROMPT "AcornOS> "

typedef struct {
    char input_buffer[SHELL_MAX_INPUT];
    char* args[SHELL_MAX_ARGS];
    int argc;
    char history[SHELL_MAX_HISTORY][SHELL_MAX_INPUT];
    int history_count;
    int history_index;
} shell_context_t;

typedef struct {
    const char* name;
    const char* description;
    void (*handler)(int argc, char* argv[]);
} shell_command_t;


void shell_init(void);
void shell_run(void);
void shell_parse_input(const char* input, shell_context_t* ctx);
void shell_execute_command(shell_context_t* ctx);
void shell_add_to_history(shell_context_t* ctx, const char* input);
void shell_print_prompt(void);
void cmd_help(int argc, char* argv[]);
void cmd_clear(int argc, char* argv[]);
void cmd_echo(int argc, char* argv[]);
void cmd_history(int argc, char* argv[]);
void cmd_about(int argc, char* argv[]);
void cmd_pwd(int argc, char* argv[]);
void cmd_cd(int argc, char* argv[]);
void cmd_ls(int argc, char* argv[]);
void cmd_touch(int argc, char* argv[]);
void cmd_rm(int argc, char* argv[]);
void cmd_mkdir(int argc, char* argv[]);
void cmd_rmdir(int argc, char* argv[]);
void cmd_cp(int argc, char* argv[]);
void cmd_mv(int argc, char* argv[]);
void cmd_cat(int argc, char* argv[]);
void cmd_stat(int argc, char* argv[]);
void cmd_fstest(int argc, char* argv[]);
void cmd_format(int argc, char* argv[]);
void cmd_mount(int argc, char* argv[]);
void cmd_echo_to_file(int argc, char* argv[]);
void cmd_edit(int argc, char* argv[]);

#endif