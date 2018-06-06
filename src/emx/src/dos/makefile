#
# /emx/src/dos/makefile
#
# Copyright (c) 1991-1999 by Eberhard Mattes
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
# along with emx; see the file COPYING.  If not, write to
# the Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#
# See emx.asm for a special exception.
#

.SUFFIXES: .obj .asm

MLOPT=
LINKOPT=
BIN=.\ #

ML=ml -c -Cp -W3 -WX -VM -nologo $(MLOPT)

.asm.obj:
	$(ML) $<

default:	all
all:		emx emxd emxl
emx:		$(BIN)emx.exe
emxd:		$(BIN)emxd.exe
emxl:		$(BIN)emxl.exe

debug:
		@$(MAKE) $(MAKEFLAGS) /NOLOGO MLOPT=-Zi LINKOPT=/CO

a20.obj:	a20.asm a20.inc emx.inc xms.inc vcpi.inc \
		options.inc

core.obj:	core.asm emx.inc oprint.inc headers.inc \
		core.inc pmint.inc signal.inc process.inc

debug.obj:	debug.asm emx.inc except.inc oprint.inc utils.inc \
		pmio.inc disasm.inc segments.inc symbols.inc \
		process.inc paging.inc tables.inc pmint.inc \
		signal.inc debug.inc swapper.inc fileio.inc

disasm.obj:	disasm.asm emx.inc symbols.inc

dpmi.obj:	dpmi.asm emx.inc rprint.inc misc.inc vcpi.inc

emx.obj:	emx.asm emx.inc oprint.inc tables.inc \
		paging.inc a20.inc segments.inc pmio.inc \
		memory.inc process.inc vprint.inc swapper.inc \
		except.inc rmint.inc debug.inc options.inc \
		version.inc vcpi.inc signal.inc pmint.inc \
		loader.inc xms.inc rprint.inc headers.inc \
		termio.inc fileio.inc utils.inc

emxl.obj:	emxl.asm emx.inc headers.inc version.inc

errors.obj:     errors.asm emx.inc errors.inc pmint.inc signal.inc \
		process.inc

except.obj:	except.asm emx.inc except.inc vprint.inc \
		swapper.inc debug.inc oprint.inc pmio.inc \
		pmint.inc tables.inc process.inc signal.inc \
		misc.inc core.inc

extapi.obj:	extapi.asm emx.inc pmint.inc

fileio.obj:	fileio.asm emx.inc signal.inc process.inc swapper.inc \
		errors.inc options.inc oprint.inc pmint.inc utils.inc

loader.obj:	loader.asm emx.inc tables.inc paging.inc \
		misc.inc signal.inc segments.inc pmio.inc \
		memory.inc process.inc symbols.inc debug.inc \
		headers.inc pmint.inc options.inc errors.inc \
		version.inc

memory.obj:	memory.asm emx.inc swapper.inc vcpi.inc \
		xms.inc memory.inc rprint.inc rmint.inc \
                paging.inc segments.inc misc.inc oprint.inc options.inc

ndebug.obj:	ndebug.asm emx.inc debug.inc pmint.inc

oprint.obj:	oprint.asm emx.inc signal.inc process.inc

options.obj:	options.asm emx.inc memory.inc a20.inc except.inc \
		pmio.inc debug.inc process.inc rprint.inc xms.inc \
		signal.inc misc.inc version.inc loader.inc

paging.obj:	paging.asm emx.inc oprint.inc memory.inc \
		vcpi.inc tables.inc segments.inc paging.inc \
		misc.inc swapper.inc

pmint.obj:	pmint.asm emx.inc vprint.inc tables.inc extapi.inc \
		vcpi.inc process.inc oprint.inc paging.inc \
		syscall.inc pmint.inc rprint.inc a20.inc profil.inc \
		debug.inc segments.inc misc.inc signal.inc \
		rmint.inc core.inc termio.inc options.inc utils.inc \
		stat.inc except.inc fileio.inc errors.inc

pmio.obj:	pmio.asm emx.inc signal.inc process.inc

process.obj:	process.asm emx.inc loader.inc process.inc errors.inc \
		signal.inc tables.inc segments.inc pmint.inc profil.inc \
		pmio.inc paging.inc fileio.inc misc.inc utils.inc

profil.obj:     profil.asm emx.inc process.inc signal.inc profil.inc \
		pmint.inc errors.inc

ptrace.obj:	ptrace.asm emx.inc pmint.inc signal.inc process.inc \
		debug.inc headers.inc errors.inc

rmint.obj:	rmint.asm emx.inc vcpi.inc rmint.inc \
		tables.inc pmint.inc except.inc rprint.inc \
		misc.inc signal.inc process.inc options.inc

rprint.obj:	rprint.asm emx.inc rprint.inc

segments.obj:	segments.asm emx.inc tables.inc paging.inc \
		segments.inc misc.inc signal.inc process.inc

signal.obj:	signal.asm emx.inc pmint.inc rmint.inc misc.inc errors.inc \
                signal.inc process.inc except.inc oprint.inc core.inc

swapper.obj:	swapper.asm emx.inc pmio.inc except.inc \
		oprint.inc segments.inc paging.inc memory.inc \
		pmint.inc process.inc tables.inc signal.inc \
		debug.inc

symbols.obj:	symbols.asm symbols.inc emx.inc process.inc \
		signal.inc

syscall.obj:	syscall.asm emx.inc process.inc paging.inc vprint.inc \
		segments.inc pmint.inc tables.inc loader.inc fileio.inc \
		except.inc swapper.inc signal.inc errors.inc vcpi.inc \
		xms.inc memory.inc misc.inc core.inc timeb.inc version.inc \
		termio.inc stat.inc ulimit.inc profil.inc utils.inc

tables.obj:	tables.asm emx.inc tables.inc process.inc \
		pmint.inc misc.inc headers.inc signal.inc \
		except.inc

termio.obj:	termio.asm emx.inc signal.inc process.inc \
		termio.inc errors.inc

utils.obj:	utils.asm emx.inc

vcpi.obj:	vcpi.asm emx.inc tables.inc pmint.inc \
		paging.inc misc.inc rprint.inc memory.inc \
		segments.inc oprint.inc

vprint.obj:	vprint.asm emx.inc tables.inc vprint.inc

xms.obj:	xms.asm xms.inc emx.inc memory.inc \
		rprint.inc vcpi.inc options.inc

EMXOBJ1 =	emx.obj tables.obj loader.obj oprint.obj utils.obj
EMXOBJ2 =	segments.obj paging.obj pmio.obj memory.obj ptrace.obj
EMXOBJ3 =	profil.obj process.obj vprint.obj swapper.obj except.obj
EMXOBJ4 =	extapi.obj rmint.obj vcpi.obj syscall.obj signal.obj
EMXOBJ5 =	fileio.obj pmint.obj rprint.obj dpmi.obj symbols.obj
EMXOBJ6 =	errors.obj xms.obj a20.obj core.obj termio.obj options.obj
EMXDEP =	$(EMXOBJ1) $(EMXOBJ2) $(EMXOBJ3) $(EMXOBJ4) $(EMXOBJ5) \
		$(EMXOBJ6) emx.lnk

emx.lnk:	makefile
	@echo $(EMXOBJ1) + >emx.lnk
	@echo $(EMXOBJ2) + >>emx.lnk
	@echo $(EMXOBJ3) + >>emx.lnk
	@echo $(EMXOBJ4) + >>emx.lnk
	@echo $(EMXOBJ5) + >>emx.lnk
	@echo $(EMXOBJ6) + >>emx.lnk

$(BIN)emx.exe:	$(EMXDEP) ndebug.obj
	link @emx.lnk ndebug, $(BIN)emx, emx /cp:1 /map $(LINKOPT);

$(BIN)emxd.exe: $(EMXDEP) debug.obj disasm.obj
	link @emx.lnk debug disasm, $(BIN)emxd, emxd /cp:1 /map $(LINKOPT);

$(BIN)emxl.exe: emxl.obj
	link emxl, $(BIN)emxl, emxl /cp:1 /map $(LINKOPT);

clean:
	-del *.obj
	-del *.map
	-del emx.lnk

realclean:
	-del $(BIN)emx.exe
	-del $(BIN)emxd.exe
	-del $(BIN)emxl.exe

# End of /emx/src/dos/makefile
