# Main development file for building and testing lua compiled

WARN_LIMIT=20
CC_WARNING=-fmax-errors=$(WARN_LIMIT) \
		   -Werror \
		   -Wextra \
		   -Wformat-overflow \
		   -Wconversion \

CC=gcc
flags=-Wall
DEBUG=-g -DDEBUG_LUA=1

.PHONY: all build

all: build

build:
	mkdir -p build
	$(CC) $(CC_WARNING) $(flags) src/*.c -o build/lua

debug: 
	mkdir -p build
	$(CC) $(CC_WARNING) $(flags) $(DEBUG) src/*.c -o build/lua
