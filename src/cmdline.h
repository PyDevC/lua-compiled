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
LuaCmdlineParam getParams();

// Display Flags in correct format
void cmdArgsDisplayFlag(char *flagname, char *description);

// Display message on stdout
void cmdArgsDisplayMsg(char *message);

// setup defines and params for cli args
void setupCmdlineArgs(int argc, char *argv[]);

// Main display function
void printCmdlineUsage();
