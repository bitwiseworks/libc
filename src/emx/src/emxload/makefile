#
# /emx/src/emxload/makefile
#
# Copyright (c) 1993-1998 by Eberhard Mattes
#
# This file is part of emxload.
#
# emxload is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# emxload is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emxload; see the file COPYING.  If not, write to the
# Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#
#
# Note: Unset GCCLOAD and run "emxload -q" before compiling emxload!
#

BIN=/emx/bin/
I=d:/emx/include/

CC=gcc
CFLAGS=-Zomf -Zmt -Zcrtdll -O -Wall -I../include

default:	all
all:		emxload
emxload:	$(BIN)emxload.exe

emxload.obj: emxload.c $(I)emx/emxload.h
	$(CC) $(CFLAGS) -c emxload.c

$(BIN)emxload.exe: emxload.obj
	-emxload -qw
	$(CC) $(CFLAGS) -o $(BIN)emxload.exe -s emxload.obj

clean:
	-rm *.obj

realclean: clean
	-rm $(BIN)emxload.exe

# End of /emx/src/emxload/makefile
