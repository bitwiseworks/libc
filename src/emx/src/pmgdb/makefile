#
# /emx/src/pmgdb/makefile
#
# Copyright (c) 1996 Eberhard Mattes
#
# This file is part of pmgdb.
#
# pmgdb is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# pmgdb is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with pmgdb; see the file COPYING.  If not, write to the
# the Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

BIN=\emx\bin\ #
HELP=\emx\help\ #
CC=gcc -g -O2 -Zmt -Wall

OBJ=pmgdb.o pmtty.o pmtxt.o pmframe.o source.o srcfiles.o command.o \
		breakpoi.o display.o threads.o register.o annotati.o \
		capture.o gdbio.o pmdebug.o pmapp.o string.o

default: $(BIN)pmgdb.exe $(HELP)pmgdb.hlp

$(BIN)pmgdb.exe:	$(OBJ) pmgdb.def pmgdb.res
	$(CC) -o pmgdb $(OBJ) pmgdb.def -Zsmall-conv
	emxbind -b -s -o$(BIN)pmgdb.exe -rpmgdb.res -dpmgdb.def pmgdb

pmgdb.o:	pmgdb.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h \
		breakpoi.h source.h command.h pmgdb.h
	$(CC) -c pmgdb.cc

command.o:	command.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h \
		breakpoi.h display.h threads.h register.h source.h srcfiles.h \
		annotati.h capture.h gdbio.h command.h pmdebug.h pmgdb.h help.h
	$(CC) -c command.cc

source.o:	source.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h \
		breakpoi.h display.h source.h annotati.h capture.h \
		gdbio.h command.h pmgdb.h help.h
	$(CC) -c source.cc

srcfiles.o:	srcfiles.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h \
		breakpoi.h srcfiles.h command.h pmgdb.h help.h
	$(CC) -c srcfiles.cc

breakpoi.o:	breakpoi.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h \
		breakpoi.h srcfiles.h annotati.h capture.h gdbio.h command.h \
		pmgdb.h help.h
	$(CC) -c breakpoi.cc

display.o:	display.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h \
		breakpoi.h display.h annotati.h capture.h gdbio.h command.h \
		pmgdb.h help.h
	$(CC) -c display.cc

threads.o:	threads.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h \
		breakpoi.h threads.h annotati.h gdbio.h command.h \
		pmgdb.h help.h
	$(CC) -c threads.cc

register.o:	register.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h \
		breakpoi.h register.h pmgdb.h help.h
	$(CC) -c register.cc

annotati.o:	annotati.cc annotati.h
	$(CC) -c annotati.cc

capture.o:	capture.cc capture.h
	$(CC) -c capture.cc

gdbio.o:	gdbio.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h \
		breakpoi.h display.h threads.h annotati.h capture.h \
		gdbio.h pmdebug.h command.h
	$(CC) -c gdbio.cc

pmdebug.o:	pmdebug.cc pmdebug.h
	$(CC) -c pmdebug.cc

pmtty.o:	pmtty.cc string.h pmapp.h pmframe.h pmtxt.h pmtty.h
	$(CC) -c pmtty.cc

pmtxt.o:	pmtxt.cc string.h pmapp.h pmframe.h pmtxt.h
	$(CC) -c pmtxt.cc

pmframe.o:	pmframe.cc string.h pmapp.h pmframe.h
	$(CC) -c pmframe.cc

pmapp.o:	pmapp.cc string.h pmapp.h
	$(CC) -c pmapp.cc

string.o:	string.cc string.h
	$(CC) -c string.cc

pmgdb.res:        pmgdb.rc pmgdb.h help.h
	rc -r pmgdb.rc

$(HELP)pmgdb.hlp:	pmgdb.ipp pmgdb.h
	ipfcprep pmgdb.ipp pmgdb.ipf
	ipfc pmgdb.ipf
	del pmgdb.ipf
	cp pmgdb.hlp $(HELP)pmgdb.hlp

clean:
	-rm -f *.o *.exe *.res *.hlp pmgdb.ipf core pmgdb
