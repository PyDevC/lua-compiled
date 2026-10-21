WARN_LIMIT=20
CC_WARNING=-fmax-errors=$(WARN_LIMIT) \
		   -Werror \
		   -Wextra \
		   -Wformat-overflow \
		   -Wconversion \

CC=gcc
flags=-Wall
DEBUG=-g -DDEBUG_LUA=1
BUILD_PATH=build

.PHONY: all build test debug fmt

all: build test

build:
	mkdir -p build
	$(CC) $(CC_WARNING) $(flags) src/*.c -O3 -o $(BUILD_PATH)/lua

debug: 
	mkdir -p build
	$(CC) $(CC_WARNING) $(flags) $(DEBUG) src/*.c -o $(BUILD_PATH)/lua

fmt:
	clang-format --style=file -i src/*

test:
	$(BUILD_PATH)/lua testfiles/assignment.lua
	$(BUILD_PATH)/lua testfiles/expressions.lua
