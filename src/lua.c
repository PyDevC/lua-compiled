#define CLUA_VERSION 0.0.1
#define CODENAME LUA_PR

#include "lexer.h"
#include "parser.h"
#include <stdio.h>

void usage() { /* Display usage of the compiler with the options available */
  printf("lua [flags] [filename]\n");
  printf("ex: lua hello.lua\n");
}

int main(int argc, char **argv) {
  if (argc == 1) {
    usage();
    return 1;
  } else if (argc == 2) {
    char *filename = argv[1];
    init_lexer(filename);
    TokenStruct token;
    do {
      token = get_next_token();
      printf("%s -> %d\n", token.literal, token.type);
      free(token.literal);
    } while (token.type != _EOF);
  }
  return 0;
}
