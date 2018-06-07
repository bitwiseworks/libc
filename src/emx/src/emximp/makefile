#
# /emx/src/emximp/makefile
#
# Copyright (c) 1992-1998 by Eberhard Mattes
#
# This file is part of emximp.
#
# emximp is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# emximp is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emximp; see the file COPYING.  If not, write to the
# Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

.SUFFIXES: .c .o

BIN=d:\emx\bin\ #
INC=..\include\ #
L=d:/emx/lib/
I=d:/emx/include/
S=$(I)sys/

OMFLIB=$(L)omflib.a
MODDEF=$(L)moddef.a

CC=gcc
CFLAGS=-O -Wall -I../include
LFLAGS=-s -Zsmall-conv

.c.o:
	$(CC) $(CFLAGS) -c $<

default:	all
all:		emximp
emximp:		$(BIN)emximp.exe

emximp.o: emximp.c $(INC)defs.h $(S)omflib.h $(S)moddef.h

$(BIN)emximp.exe: emximp.o $(OMFLIB) $(MODDEF)
	gcc $(LFLAGS) -o $(BIN)emximp.exe emximp.o -lomflib -lmoddef

clean:
	-del *.o

realclean: clean
	-del $(BIN)emximp.exe

# End of /emx/src/emximp/makefile
