CC=gcc
flags=-Werror

com:
	mkdir -p build
	gcc src/*.c -o build/lua

debug:
	mkdir -p build
	gcc -g -Wall src/*.c -o build/lua
