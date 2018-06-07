#	@(#)Makefile	5.10 (Berkeley) 6/24/90

CC=	gcc -O
CFLAGS= -I. -DOS2

.c.o:
	$(CC) -c $(CFLAGS) $<

SRCS=	addbytes.c addch.c addstr.c box.c clear.c clrtobot.c clrtoeol.c \
	cr_put.c cr_tty.c curses.c delch.c deleteln.c delwin.c endwin.c \
	erase.c fullname.c getch.c getstr.c idlok.c id_subwi.c initscr.c \
	insch.c insertln.c longname.c move.c mvprintw.c mvscanw.c mvwin.c \
	newwin.c overlay.c overwrit.c printw.c putchar.c refresh.c scanw.c \
	scroll.c touchola.c standout.c touchwin.c tstp.c unctrl.c os2.c
MAN3=	curses.0

OBJS=   $(SRCS:.c=.o)

curses.a: $(OBJS)
	-del $@
	ar r $@ $(OBJS)
	ar s $@
