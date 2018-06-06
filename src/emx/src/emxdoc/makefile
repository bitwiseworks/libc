#
# /emx/src/emxdoc/makefile
#
# Copyright (c) 1993-1999 by Eberhard Mattes
#
# This file is part of emxdoc.
#
# emxdoc is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# emxdoc is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emxdoc; see the file COPYING.  If not, write to the
# Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

.SUFFIXES: .c .o

BIN=d:\emx\bin\ #

CC=gcc
CFLAGS=-O -Wall
LFLAGS=-s -Zsmall-conv
#CFLAGS=-g -Wall
#LFLAGS=-g

.c.o:
	$(CC) $(CFLAGS) -c $<

default:	all
all:		emxdoc
emxdoc:		$(BIN)emxdoc.exe

emxdoc.o: emxdoc.c emxdoc.h html.h ipf.h latex.h text.h xref.h cond.h
input.o:  input.c emxdoc.h cond.h
cond.o:   cond.c emxdoc.h cond.h
html.o:   html.c emxdoc.h html.h xref.h
ipf.o:    ipf.c emxdoc.h ipf.h xref.h
latex.o:  latex.c emxdoc.h latex.h
text.o:   text.c emxdoc.h lb.h text.h
xref.o:   xref.c emxdoc.h xref.h
lb.o:    lb.c lb.h

$(BIN)emxdoc.exe: emxdoc.o input.o cond.o html.o ipf.o latex.o text.o \
		xref.o lb.o
	gcc $(LFLAGS) -o $(BIN)emxdoc.exe emxdoc.o input.o cond.o \
		html.o ipf.o latex.o text.o xref.o lb.o


clean:
	-del *.o

realclean: clean
	-del $(BIN)emxdoc.exe

# End of /emx/src/emxdoc/makefile
