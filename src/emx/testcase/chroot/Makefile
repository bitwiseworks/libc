all: chroot-1.exe

%.exe: %.c
	gcc -g -Zomf -Zmap -o $@ $^

test: all
	./test-chroot-1.sh
