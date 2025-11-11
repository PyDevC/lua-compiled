# Main development file for building and testing lua compiled

WARN_LIMIT=20
CC_WARNING=-fmax-errors=$(WARN_LIMIT) \
		   -Werror \
		   -Wextra \
		   -Wformat-overflow \
		   -Wconversion \

CC=gcc
flags=-Wall
DEBUG=-g 

all: set com

set:
	mkdir -p build

com:
	$(CC) $(CC_WARNING) $(flags) src/*.c -o build/lua

debug:
	$(CC) $(CC_WARNING) $(flags) $(DEBUG) src/*.c -o build/lua
