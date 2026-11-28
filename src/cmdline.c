#include "cmdline.h"

#include <stdio.h>
#include <stdlib.h>

LuaCmdlineParam params;

void setupDefineFlags()
{
    if (params.debug) {
#define LUA_DEBUG 1
    }
    if (params.output_bin_name == NULL) {
        params.output_bin_name = "out.o";
    }
}

LuaCmdlineParam getParams() { return params; }

void cmdArgsDisplayFlag(char *flagname, char *description)
{
    fprintf(stdout, "  %s \t%s\n", flagname, description);
}

void cmdArgsDisplayMsg(char *message) { fprintf(stdout, "%s\n", message); }

void printCmdlineUsage()
{
    cmdArgsDisplayMsg("luac [-flags] filename.lua");
    cmdArgsDisplayFlag("-o", "name of target binary");
    cmdArgsDisplayFlag("-c", "execute lua oneline command");
    cmdArgsDisplayFlag("-h", "display this help");
    cmdArgsDisplayFlag("-g", "debug lua file");
    fflush(stdout);
}

void setupCmdlineArgs(int argc, char *argv[])
{
    int arg = 0;
    for (; arg < argc; arg++) {
        char *param = argv[arg];
        // is a flag?
        if (param[0] == '-') {
            if (param[2] != '\0') {
                fprintf(stderr, "Flag not supported");
                exit(1);
            } else {
                switch (param[1]) {
                case 'c': {
                    params.mode = CMDLINE;
                } break;
                case 'h': {
                    printCmdlineUsage();
                    exit(0);
                } break;
                case 'o': {
                    params.output_bin_name = argv[++arg];
                } break;
                case 'g': {
                    params.debug = true;
                } break;
                default:
                    cmdArgsDisplayMsg("flag does not exits");
                    printCmdlineUsage();
                };
            }
        } else {
            params.filename = argv[arg];
            params.mode = LUAFILE;
        }
    }
    setupDefineFlags();
}
