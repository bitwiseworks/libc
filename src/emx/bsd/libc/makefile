# /emx/bsd/libc/makefile

CC=	gcc -O
CFLAGS= -I.

.c.o:
	$(CC) -c $(CFLAGS) $<
default: bsd.a

bsd.a: heapsort.o random.o strsep.o
	-del $@
	ar r $@ *.o
	ar s $@
