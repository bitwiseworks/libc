; $Id: micro.asm 808 2003-10-06 00:23:40Z bird $
;; @file
;
; The quickest possible executable testcase.
; Used as reference.
;
; Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
;
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with This program; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;
;

    .386

CODE32 segment public para use32 'CODE'
CODE32 ends
DATA32 segment public para use32 'DATA'
DATA32 ends
BSS32  segment public para use32 'BSS'
BSS32  ends
DGROUP group DATA32,BSS32

CODE32 segment public para use32 'CODE'
;    ASSUME cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT
    public entry
entry:
    xor     eax, eax
    ret
CODE32 ends

STACK32 segment stack para use32 'STACK'
    db 4096 dup(?)
STACK32 ends

end entry

