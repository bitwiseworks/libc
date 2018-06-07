#
# /emx/src/emxcat/makefile
#
# Copyright (c) 1992-1998 by Eberhard Mattes
#
# This file is part of emxcat.
#
# emxcat is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# emxcat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emxcat; see the file COPYING.  If not, write to the
# Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

.SUFFIXES: .c .o

BIN=d:\emx\bin\ #

CC=gcc
CFLAGS=-O -Wall
LFLAGS=-s -Zsmall-conv

.c.o:
	$(CC) $(CFLAGS) -c $<

default:	all
all:		emxcat
emxcat:		$(BIN)emxcat.exe

emxcat.o: emxcat.c

$(BIN)emxcat.exe: emxcat.o
	gcc $(LFLAGS) -o $(BIN)emxcat.exe emxcat.o

clean:
	-del *.o

realclean: clean
	-del $(BIN)emxcat.exe

# End of /emx/src/emxcat/makefile
