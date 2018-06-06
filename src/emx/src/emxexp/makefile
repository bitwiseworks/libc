ifndef OLD_EMX_MAKEFILE
.c.o:
	$(MAKE) -C ../.. OPT=dbg out/dbg/omf/src/emxexp/$@
else
#
# /emx/src/emxexp/makefile
#
# Copyright (c) 1993-1998 by Eberhard Mattes
#
# This file is part of emxexp.
#
# emxexp is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# emxexp is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emxexp; see the file COPYING.  If not, write to the
# Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

BIN=/emx/bin/

DEFS_H=../include/defs.h
OMFLIB_H=d:/emx/include/sys/omflib.h

OMFLIB=d:/emx/lib/omflib.a

CC=gcc
CFLAGS=-O -Wall -I../include -I.
LFLAGS=-s -Zsmall-conv

default:	all
all:		emxexp
emxexp:	$(BIN)emxexp.exe

emxexp.o: emxexp.c $(DEFS_H) $(OMFLIB_H)
	$(CC) $(CFLAGS) -c emxexp.c

cplus-de.o: cplus-de.c demangle.h ansidecl.h libibert.h
	$(CC) $(CFLAGS) -c cplus-de.c

$(BIN)emxexp.exe: emxexp.o cplus-de.o $(OMFLIB)
	$(CC) $(LFLAGS) -o $(BIN)emxexp.exe emxexp.o cplus-de.o $(OMFLIB)

clean:
	-rm *.o

realclean: clean
	-rm $(BIN)emxexp.exe

# End of /emx/src/emxexp/makefile
endif
