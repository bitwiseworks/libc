/* reg.h -- Register numbers
   Copyright (c) 1992-1995 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


/* This header contains the macros of <sys/reg.h> prepended with
   "R_" to avoid conflicts with the members of uDB_t. */

#define R_GS      0
#define R_FS      1
#define R_ES      2
#define R_DS      3
#define R_EDI     4
#define R_ESI     5
#define R_EBP     6
#define R_ESP     7
#define R_EBX     8
#define R_EDX     9
#define R_ECX    10
#define R_EAX    11
#define R_EIP    14
#define R_CS     15
#define R_EFL    16
#define R_UESP   17
#define R_SS     18
