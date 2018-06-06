#
# termcap/Makefile
#
default: termcap.a

CFLAGS = -O -I. -DHAVE_CONFIG_H

.c.o:
	gcc -c $(CFLAGS) $<

termcap.a: termcap.o tparam.o version.o
	-del termcap.a
	ar r termcap.a termcap.o tparam.o version.o
	ar s termcap.a
