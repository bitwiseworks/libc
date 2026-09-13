all: chroot-1.exe

%.exe: %.c
	gcc -g -Zomf -Zmap -o $@ $^

test: chroot-1.exe
	-chroot-1.exe

test2: chroot-1.exe
	-unset UNIXROOT && chroot-1.exe
