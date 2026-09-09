#include "cmdline.h"

#include <stdio.h>
#include <stdlib.h>

Param_T params;

void setup_define_flags()
{
    if (params.debug) {
#define LUA_DEBUG 1
    }
    if (params.output_bin_name == NULL) {
        params.output_bin_name = "out.o";
    }
}

Param_T get_params() { return params; }

void cmd_args_display_flag(char *flagname, char *description)
{
    fprintf(stdout, "  %s \t%s\n", flagname, description);
}

void cmd_args_display_msg(char *message) { fprintf(stdout, "%s\n", message); }

void print_cmdline_usage()
{
    cmd_args_display_msg("luac [-flags] filename.lua");
    cmd_args_display_flag("-o", "name of target binary");
    cmd_args_display_flag("-c", "execute lua oneline command");
    cmd_args_display_flag("-h", "display this help");
    cmd_args_display_flag("-g", "debug lua file");
    fflush(stdout);
}

void setup_cmdline_args(int argv, char *argc[])
{
    int arg = 0;
    for (; arg < argv; arg++) {
        char *param = argc[arg];
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
                    print_cmdline_usage();
                    exit(0);
                } break;
                case 'o': {
                    params.output_bin_name = argc[arg++];
                } break;
                case 'g': {
                    params.debug = true;
                } break;
                default:
                    cmd_args_display_msg("flag does not exits");
                    print_cmdline_usage();
                };
            }
        } else {
            params.filename = argc[arg];
            params.mode = LUAFILE;
        }
    }
    setup_define_flags();
}
