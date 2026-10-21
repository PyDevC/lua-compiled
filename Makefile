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

TEST_SRCS = $(wildcard tests/test_*.c)
TEST_BINS = $(TEST_SRCS:tests/%.c=$(BUILD_PATH)/%)

LIB_SRCS = $(filter-out src/lua.c, $(wildcard src/*.c))
LIB_OBJS = $(LIB_SRCS:src/%.c=$(BUILD_PATH)/%.o)

MUNIT_SRC = thirdparty/munit/munit.c
MUNIT_OBJ = $(BUILD_PATH)/munit.o
INCLUDES = -Isrc -Ithirdparty/munit -Itests

.PHONY: all build test unit-test script-test debug fmt clean

all: build test

build:
	mkdir -p $(BUILD_PATH)
	$(CC) $(CC_WARNING) $(flags) src/*.c -O3 -o $(BUILD_PATH)/lua

debug: 
	mkdir -p $(BUILD_PATH)
	$(CC) $(CC_WARNING) $(flags) $(DEBUG) src/*.c -o $(BUILD_PATH)/lua

fmt:
	clang-format --style=file -i src/*

test: script-test unit-test

script-test:
	$(BUILD_PATH)/lua testfiles/assignment.lua
	$(BUILD_PATH)/lua testfiles/expressions.lua
	$(BUILD_PATH)/lua testfiles/controlflow.lua
	$(BUILD_PATH)/lua testfiles/loops.lua

unit-test: $(TEST_BINS)
	@for t in $(TEST_BINS); do \
		echo "Running $$t..."; \
		./$$t || exit 1; \
	done

$(MUNIT_OBJ): $(MUNIT_SRC)
	mkdir -p $(BUILD_PATH)
	$(CC) -std=c11 -Ithirdparty/munit -DATOMIC_VAR_INIT\(x\)=\(x\) -c $< -o $@

$(BUILD_PATH)/%.o: src/%.c
	mkdir -p $(BUILD_PATH)
	$(CC) $(CC_WARNING) $(flags) $(DEBUG) -Isrc -c $< -o $@

$(BUILD_PATH)/test_%: tests/test_%.c $(LIB_OBJS) $(MUNIT_OBJ)
	mkdir -p $(BUILD_PATH)
	$(CC) $(CC_WARNING) $(flags) $(DEBUG) $^ $(INCLUDES) -o $@

clean:
	rm -rf $(BUILD_PATH)
