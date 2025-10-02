#include <stdio.h>

// File related errors
void file_not_found(const char *filename) {
  printf("%s not found, No such file or directory", filename);
}
