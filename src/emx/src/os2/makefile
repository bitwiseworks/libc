#
# /emx/src/os2/makefile
#
# Copyright (c) 1992-1998 by Eberhard Mattes
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
# As special exception, emx.dll can be distributed without source code
# unless it has been changed.  If you modify emx.dll, this exception
# no longer applies and you must remove this paragraph from all source
# files for emx.dll.
#


.SUFFIXES: .obj .asm .c

DLL=\emx\dll\ #
#DLL=\emx\test\ #
I=d:\emx\include\ #
MAP=d:\emx\etc\ #
LIB=d:\emx\lib\ #
LIBS=c.lib $(LIB)mt\sys.lib $(LIB)os2.lib

ML=ml -c -Cp -W3 -WX -VM -nologo -Zi
CC=gcc
CFLAGS=-c -mprobe -fstack-check -O -g -Wall -Werror

.asm.obj:
	$(ML) $<

.c.obj:
	$(CC) $(CFLAGS) $<
	emxomf -d $*.o

default:	all
all:		emx emxio
emx:		$(DLL)emx.dll
emxio:		$(DLL)emxio.dll

c.lib:	$(LIB)mt\c.lib
	copy $(LIB)mt\c.lib c.lib
	emxomfar d c.lib crt1 nlsctype

OBJECTS =	emxdll.obj doscall.obj syscall.obj init.obj memory.obj \
		process.obj select.obj signal.obj time.obj fileio.obj \
		ptrace.obj core.obj termio.obj keyboard.obj errors.obj \
		utils.obj tcpip.obj pm.obj wrapper.obj \
		xf86sup.obj profil.obj

$(DLL)emx.dll:	$(OBJECTS) $(LIBS) emxdll.def
	link386 /nol /noi $(OBJECTS), $(DLL)emx.dbg, $(MAP)emx /map, $(LIBS), \
		emxdll /nod /deb
	-emxload -qw
	link386 /nol /noi $(OBJECTS), $(DLL)emx.dll, $(MAP)emx /map, $(LIBS), \
		emxdll /nod

emxdll.obj:	emxdll.asm emxdll.inc

core.obj: core.c emxdll.h $(I)sys/user.h reg.h clib.h

doscall.obj: doscall.c emxdll.h

syscall.obj: syscall.c emxdll.h tcpip.h version.h $(I)sys/timeb.h \
		$(I)sys/ulimit.h $(I)sys/nls.h

fileio.obj: fileio.c emxdll.h files.h $(I)sys/fcntl.h $(I)sys/errno.h \
		clib.h $(I)sys/types.h $(I)sys/stat.h $(I)sys/ioctl.h \
		$(I)sys/termio.h $(I)sys/time.h $(I)emx/syscalls.h \
		$(I)sys/nls.h

init.obj: init.c emxdll.h clib.h version.h $(I)emx/startup.h

memory.obj: memory.c emxdll.h $(I)sys/builtin.h $(I)sys/fmutex.h

pm.obj: pm.c emxdll.h

process.obj: process.c emxdll.h files.h clib.h \
		$(I)sys/signal.h $(I)sys/errno.h $(I)sys/wait.h \
		$(I)sys/process.h $(I)sys/ptrace.h $(I)emx/syscalls.h \
		$(I)sys/uflags.h

ptrace.obj: ptrace.c $(I)sys/signal.h $(I)sys/ptrace.h $(I)sys/user.h \
		reg.h $(I)sys/errno.h $(I)sys/uflags.h emxdll.h clib.h

select.obj: select.c emxdll.h files.h select.h tcpip.h clib.h \
		$(I)sys/termio.h $(I)sys/errno.h $(I)sys/types.h \
		$(I)emx/syscalls.h

signal.obj: signal.c emxdll.h wrapper.h \
		$(I)sys/signal.h $(I)sys/errno.h $(I)sys/uflags.h \
		$(I)emx/syscalls.h

tcpip.obj: tcpip.c emxdll.h files.h tcpip.h select.h clib.h $(I)sys/errno.h \
		$(I)sys/types.h $(I)sys/stat.h $(I)sys/ioctl.h \
		$(I)sys/fcntl.h $(I)sys/socket.h $(I)sys/so_ioctl.h \
		$(I)emx/syscalls.h \
		$(I)net/route.h $(I)net/if.h $(I)net/if_arp.h $(I)netdb.h

termio.obj: termio.c emxdll.h files.h clib.h $(I)sys/errno.h $(I)sys/fcntl.h \
		$(I)sys/signal.h $(I)sys/ioctl.h $(I)sys/termio.h $(I)termios.h

keyboard.obj: keyboard.c emxdll.h clib.h

time.obj: time.c emxdll.h $(I)limits.h $(I)sys/timeb.h $(I)sys/time.h

errors.obj: errors.c emxdll.h $(I)sys/errno.h

utils.obj: utils.c emxdll.h clib.h $(I)sys/nls.h

xf86sup.obj: xf86sup.c xf86sup.h files.h select.h emxdll.h clib.h \
		$(I)sys/errno.h $(I)sys/fcntl.h $(I)sys/ioctl.h \
		$(I)sys/so_ioctl.h $(I)sys/types.h $(I)sys/termio.h \
		$(I)termios.h

profil.obj: profil.c profil.h emxdll.h clib.h $(I)emx/syscalls.h

wrapper.obj: wrapper.c wrapper.h $(I)os2thunk.h

$(DLL)emxio.dll: emxio.obj emxio.def
	link386 /nol emxio, $(DLL)emxio.dll, $(MAP)emxio /map,, emxio

emxio.obj: emxio.asm emxdll.inc

clean:
	-del *.obj
	-del c.lib

realclean: clean

# End of /emx/src/os2/makefile
