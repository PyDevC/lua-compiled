#pragma once

#include <stdbool.h>

typedef enum { CMDLINE, LUAFILE, MODULE } RunMode;

typedef struct LuaCmdlineParam LuaCmdlineParam;
extern LuaCmdlineParam params;

struct LuaCmdlineParam
{
    char *filename;
    char *output_bin_name;
    bool debug;
    RunMode mode;
};

// Get output_bin_name
LuaCmdlineParam get_params();

// Display Flags in correct format
void cmd_args_display_flag(char *flagname, char *description);

// Display message on stdout
void cmd_args_display_msg(char *message);

// setup defines and params for cli args
void setup_cmdline_args(int argc, char *argv[]);

// Main display function
void print_cmdline_usage();
