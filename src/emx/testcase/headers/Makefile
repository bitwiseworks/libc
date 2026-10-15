MAKEFLAGS += -k

all: posix-socket with-underscore-stdlib

%: %.c
	gcc -std=c99 -fsyntax-only -I../../include $< >$@.log 2>&1
