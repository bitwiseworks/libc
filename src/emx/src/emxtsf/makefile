#
# /emx/src/emxtsf/makefile
#
# Copyright (c) 1996 by Eberhard Mattes
#
# This file is part of emxtsf.
#
# emxtsf is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# emxtsf is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emxtsf; see the file COPYING.  If not, write to the
# Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

.SUFFIXES: .c .o

BIN=/emx/bin/

CC=gcc
CFLAGS=-O -Wall
LFLAGS=-s -Zsmall-conv

.c.o:
	$(CC) $(CFLAGS) -c $<

default:	all
all:		emxtsf
emxtsf:		$(BIN)emxtsf.exe

$(BIN)emxtsf.exe: emxtsf.o
	gcc $(LFLAGS) -o $(BIN)emxtsf.exe emxtsf.o -lmoddef

emxtsf.o: emxtsf.c

clean:
	-rm *.o

realclean: clean
	-del $(BIN)emxtsf.exe

# End of /emx/src/emxtsf/makefile
