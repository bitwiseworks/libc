;
; STAT.INC -- Assembler version of sys/stat.h
;
; Copyright (c) 1991-1995 by Eberhard Mattes
;
; This file is part of emx.
;
; emx is free software; you can redistribute it and/or modify it
; under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2, or (at your option)
; any later version.
;
; emx is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with emx; see the file COPYING.  If not, write to
; the Free Software Foundation, 59 Temple Place - Suite 330,
; Boston, MA 02111-1307, USA.
;
; See emx.asm for a special exception.
;

S_IREAD         =       0400Q                   ; Owner: read
S_IWRITE        =       0200Q                   ; Owner: write
S_IEXEC         =       0100Q                   ; Owner: exec

S_IFMT          =       0160000Q                ; Mask for file type
S_IFCHR         =       0020000Q                ; Character device
S_IFDIR         =       0040000Q                ; Directory
S_IFREG         =       0100000Q                ; Regular file
S_IFSOCK        =       0140000Q                ; Socket (or pipe)

STAT            STRUCT
ST_DEV          DD      ?
ST_INO          DD      ?
ST_MODE         DD      ?
ST_NLINK        DD      ?
ST_UID          DD      ?
ST_GID          DD      ?
ST_RDEV         DD      ?
ST_SIZE         DD      ?
ST_ATIME        DD      ?
ST_MTIME        DD      ?
ST_CTIME        DD      ?
ST_ATTR         DD      ?
ST_RESERVED     DD      ?
STAT            ENDS
