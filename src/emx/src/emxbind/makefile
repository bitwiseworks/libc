#
# /emx/src/emxbind/makefile
#
# Copyright (c) 1991-1998 by Eberhard Mattes
#
# This file is part of emxbind.
#
# emxbind is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# emxbind is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emxbind; see the file COPYING.  If not, write to the
# Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

.SUFFIXES: .c .o

BIN=d:\emx\bin\ #
LIB=\emx\lib\ #
INC=..\include\ #
L=d:/emx/lib/
I=d:/emx/include/
S=$(I)sys/

MODDEF=$(L)moddef.a

H=$(INC)defs.h emxbind.h

CC=gcc
CFLAGS=-O -Wall -I../include -DLIST_OPT=1
LFLAGS=-s -Zsmall-conv

.c.o:
	$(CC) $(CFLAGS) -c $<

default:	all
all:		emxbind
emxbind:	$(BIN)emxbind.exe

OBJS = emxbind.o exec.o fixup.o export.o resource.o cmd.o list.o map.o utils.o

emxbind.o:  emxbind.c $(H) $(S)moddef.h
exec.o:     exec.c $(H) $(S)moddef.h $(S)user.h
fixup.o:    fixup.c $(H)
export.o:   export.c $(H)
resource.o: resource.c $(H)
cmd.o:      cmd.c $(H)
list.o:     list.c $(H)
map.o:      map.c $(H)
utils.o:    utils.c $(H)

$(BIN)emxbind.exe: $(OBJS) $(MODDEF)
	gcc $(LFLAGS) -o $(BIN)emxbindn.exe $(OBJS) -lmoddef
	del $(BIN)emxbind.exe
	ren $(BIN)emxbindn.exe emxbind.exe

clean:
	-del *.o

# End of /emx/src/emxbind/makefile
