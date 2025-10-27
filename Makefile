CC=gcc
flags=-Werror

com:
	mkdir -p build
	gcc src/*.c -o build/lua

debug:
	mkdir -p build
	gcc -g src/*.c -o build/lua
