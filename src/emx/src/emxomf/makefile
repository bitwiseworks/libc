ifndef OLD_EMX_MAKEFILE
.c.o:
	$(MAKE) -C ../.. OPT=dbg out/dbg/omf/src/emxomf/$@
else
#
# /emx/src/emxomf/makefile
#
# Copyright (c) 1992-1998 Eberhard Mattes
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
# the Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

.SUFFIXES: .c .o

BIN=\emx\bin\ #
I=d:/emx/include/
S=$(I)sys/
L=d:/emx/lib/

OMFLIB=$(L)omflib.a
DEFS_H=../include/defs.h
CC=gcc
CFLAGS=-c -O -Wall -I../include
LFLAGS=-s -Zsmall-conv

.c.o:
	$(CC) $(CFLAGS) $<

default:	all
all:		emxomf emxomfar emxomfld emxaout listomf
emxomf:		$(BIN)emxomf.exe
emxomfar:	$(BIN)emxomfar.exe
emxomfld:	$(BIN)emxomfld.exe
emxaout:	$(BIN)emxaout.exe
listomf:	$(BIN)listomf.exe

$(BIN)emxomf.exe: emxomf.o stabshll.o grow.o $(OMFLIB)
	-emxload -uw $(BIN)emxomf
	gcc $(LFLAGS) -o $(BIN)emxomf.exe emxomf.o stabshll.o grow.o -lomflib

$(BIN)emxomfar.exe: emxomfar.o $(OMFLIB)
	-emxload -uw $(BIN)emxomfar
	gcc $(LFLAGS) -o $(BIN)emxomfar.exe emxomfar.o -lomflib

$(BIN)emxomfld.exe: emxomfld.o $(L)moddef.a
	-emxload -uw $(BIN)emxomfld
	gcc $(LFLAGS) -o $(BIN)emxomfld.exe emxomfld.o -lmoddef
	emxbind -qa $(BIN)emxomfld.exe -p

$(BIN)emxaout.exe: emxaout.o grow.o
	-emxload -uw $(BIN)emxaout
	gcc $(LFLAGS) -o $(BIN)emxaout.exe emxaout.o grow.o

$(BIN)listomf.exe: listomf.o
	gcc $(LFLAGS) -o $(BIN)listomf.exe listomf.o

emxomf.o: emxomf.c emxomf.h stabshll.h grow.h $(DEFS_H) $(S)omflib.h
stabshll.o: stabshll.c emxomf.h stabshll.h grow.h $(DEFS_H)
grow.o: grow.c grow.h $(DEFS_H)
emxomfar.o: emxomfar.c $(S)omflib.h
emxomfld.o: emxomfld.c $(DEFS_H) $(S)moddef.h
emxaout.o: emxaout.c grow.h $(DEFS_H)
listomf.o: listomf.c $(DEFS_H)

clean:
	-del *.o

realclean:
	-del $(BIN)emxomf.exe
	-del $(BIN)emxomfar.exe
	-del $(BIN)emxomfld.exe
	-del $(BIN)emxaout.exe
	-del $(BIN)listomf.exe

# End of /emx/src/emxomf/makefile
endif
