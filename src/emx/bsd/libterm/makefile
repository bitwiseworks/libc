#
# termcap/Makefile
#
default: termcap.a

CFLAGS = -O -DEMX

.c.o:
	gcc -c $(CFLAGS) $<

termcap.a: termcap.o tgoto.o tputs.o
	-del termcap.a
	ar r termcap.a termcap.o tgoto.o tputs.o
	ar s termcap.a
