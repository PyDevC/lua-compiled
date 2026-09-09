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
	mkdir -p build

build:
	$(CC) $(CC_WARNING) $(flags) src/*.c -O3 -o $(BUILD_PATH)/lua

debug: 
	$(CC) $(CC_WARNING) $(flags) $(DEBUG) src/*.c -o $(BUILD_PATH)/lua

fmt:
	clang-format --style=file -i src/*
