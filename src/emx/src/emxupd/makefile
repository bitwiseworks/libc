#
# /emx/src/emxupd/makefile
#
# Copyright (c) 1996 by Eberhard Mattes
#
# This file is part of emxupd.
#
# emxupd is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# emxupd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emxupd; see the file COPYING.  If not, write to the
# Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

BIN=/emx/bin/

CC=gcc
CFLAGS=-O -Zomf -Zsys -Wall
LFLAGS=-s -Zsmall-conv $(CFLAGS)

default:	all
all:		emxupd
emxupd:	$(BIN)emxupd.exe

emxupd.obj: emxupd.c
	$(CC) $(CFLAGS) -c emxupd.c

$(BIN)emxupd.exe: emxupd.obj
	$(CC) $(LFLAGS) -o $(BIN)emxupd.exe emxupd.obj

clean:
	-rm *.obj

realclean: clean
	-rm $(BIN)emxupd.exe

# End of /emx/src/emxupd/makefile
