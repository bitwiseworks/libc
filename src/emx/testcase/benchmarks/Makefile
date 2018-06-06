# $Id: Makefile 1006 2004-01-18 10:01:17Z bird $
## @file
#
# Makefile for the benchmark.
#
# Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
#
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with This program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#


ifdef USE_VAC
#
# VAC setup.
#
CCFLAGS			= /Ti- -O
CCLD_STATIC 	= icc $(CCFLAGS) /Gd-
CCLD_DYNAMIC 	= icc $(CCFLAGS) /Gd
E				= .exe
O				= .obj
TRG				= /Fo$@
endif

ifdef USE_EMX
#
# EMX setup.
#
CCFLAGS			= -s -O2 -Zomf -D__OS2__
CCLD_STATIC 	= gcc $(CCFLAGS)
CCLD_DYNAMIC 	= gcc $(CCFLAGS) -Zcrtdll
E				= .exe
O				= .o
TRG				= -o $@
endif

ifdef USE_LINUX
#
# Linux GCC setup.
#
CCFLAGS			= -s -O3 -D__NOTPC__
CCLD_STATIC 	= gcc $(CCFLAGS) -static
CCLD_DYNAMIC 	= gcc $(CCFLAGS)
E				=
O				= .o
TRG				= -o $@
endif


ifndef TRG
#
# Innotek GCC setup.
#
CCFLAGS			= -s -O3 -Zomf
CCLD_STATIC 	= gcc $(CCFLAGS) -static
CCLD_DYNAMIC 	= gcc $(CCFLAGS)
E				= .exe
O				= .o
TRG				= -o $@
endif



#
# Main rules.
#
all: startup1 micro$E benchmark$E

check: all
	./benchmark$E

clean:
	rm -f startup1-static$E startup1-dynamic$E micro$E benchmark$E
ifneq ($O,)
	rm -f *$O
endif	


#
# micro - calibrator.
#			
micro.exe: micro.asm
	alp -Sv:alp -Mb micro.asm
	ilink /PMTYPE:VIO /STUB:os2stub.bin /nologo micro.obj

#
# startup1.c - static and dynamic version.	
#
.PHONY: startup1
startup1: startup1-static$E startup1-dynamic$E

startup1-static$E: startup1.c
	$(CCLD_STATIC) $(TRG) $<
startup1-dynamic$E: startup1.c
	$(CCLD_DYNAMIC) $(TRG) $<


#
# Benchmark driver.
#
benchmark$E: benchmark.c PrfTiming.h
	$(CCLD_DYNAMIC) $(TRG) benchmark.c

