CC ?= gcc
CFLAGS = -std=c11 -O3 -Wall -Wextra -Werror
DEBUG_FLAGS = -std=c11 -g -O0 -Wall -Wextra -Werror -DDEBUG_LUA=1
BUILD_PATH = build
BENCH_ARGS ?=

TESTFILES = assignment expressions controlflow loops function imports smoke big

ifneq ($(filter-out assignment expressions controlflow loops function imports smoke big,$(TESTFILES)),)
$(error TESTFILES contains an unknown fixture name)
endif

export BENCH_ARGS

.PHONY: all build debug test clean bench bench-clean

all: build test

build:
	mkdir -p $(BUILD_PATH)
	$(CC) $(CFLAGS) src/lua.c -o $(BUILD_PATH)/lua

debug:
	mkdir -p $(BUILD_PATH)
	$(CC) $(DEBUG_FLAGS) src/lua.c -o $(BUILD_PATH)/lua

test: build
	@for name in $(TESTFILES); do \
		echo "compile ok: testfiles/$$name.lua"; \
		$(BUILD_PATH)/lua -v -s $(BUILD_PATH)/$$name.s testfiles/$$name.lua || exit 1; \
		$(CC) $(BUILD_PATH)/$$name.s -o $(BUILD_PATH)/$$name || exit 1; \
		echo "run ok: testfiles/$$name.lua"; \
		$(BUILD_PATH)/$$name || exit 1; \
	done

bench: build
	$(MAKE) -C benchmark run

bench-clean:
	$(MAKE) -C benchmark clean

clean: bench-clean
	rm -rf $(BUILD_PATH)
