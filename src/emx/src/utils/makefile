#
# /emx/src/utils/makefile
#
# Copyright (c) 1994-1996 by Eberhard Mattes
#
# This file is part of emx.
#
# emx is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# emx is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emx; see the file COPYING.  If not, write to the
# Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

.SUFFIXES: .c .o

BIN=\emx\bin\ #

CC=gcc
CFLAGS=-O -Wall -I../include
LFLAGS=-s -Zsmall-conv

.c.o:
	$(CC) $(CFLAGS) -c $<

default:	all
all:		updt touch
updt:		$(BIN)updt.exe
touch:		$(BIN)touch.exe

updt.o: updt.c
touch.o: touch.c

$(BIN)touch.exe: touch.o
	gcc $(LFLAGS) -o $(BIN)touch.exe touch.o

$(BIN)updt.exe: updt.o
	gcc $(LFLAGS) -o $(BIN)updt.exe updt.o

clean:
	-del *.o

realclean: clean
	-del $(BIN)touch.exe
	-del $(BIN)updt.exe

# End of /emx/src/utils/makefile
