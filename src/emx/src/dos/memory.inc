;
; MEMORY.INC -- Manage memory
;
; Copyright (c) 1991-1999 by Eberhard Mattes
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

NULL_PHYS       =       0FFFFFFFFH      ; Physical address n/a (end of chain)
NULL_RM         =       0               ; Segment n/a (end of chain)

HIMEM_MAX       =       16

;
; Note: all blocks are of size 4K and are 4K-page-aligned.
;
MEMORY_BLOCK    STRUCT
NEXT_BLOCK      DD      NULL_PHYS       ; Physical address of next block
MEMORY_BLOCK    ENDS

HIMEM_ENTRY     STRUCT
HM_ADDR         DD      ?       ; Physical address (w/o alignment)
HM_SIZE         DD      ?       ; Size of block in bytes (w/o alignment)
HM_USED         DD      ?       ; Used up to this phys addr (with alignment)
HM_AVAIL        DD      ?       ; This many bytes available (with alignment)
HM_HANDLE       DW      ?       ; For XMS
HIMEM_ENTRY     ENDS

              IFNDEF __MEMORY

SV_DATA         SEGMENT

                EXTRN   LOMEM_HEAD_PHYS:DWORD
                EXTRN   HIMEM_HEAD_PHYS:DWORD
                EXTRN   DISABLE_EXT_MEM:BYTE    ; Don't use memory above 1M
                EXTRN   DISABLE_LOW_MEM:BYTE    ; Don't use memory below 1M
                EXTRN   VDISK_FLAG:BYTE         ; VDISK.SYS 3.3 detected
                EXTRN   HIMEM_TAB:HIMEM_ENTRY
                EXTRN   HIMEM_COUNT:WORD
                EXTRN   TAVAIL:DWORD

SV_DATA         ENDS


SV_CODE         SEGMENT

                EXTRN   PM_ALLOC:NEAR
                EXTRN   PM_ALLOC_NOSWAP:NEAR
                EXTRN   PM_AVAIL:NEAR

SV_CODE         ENDS


INIT_CODE       SEGMENT

                EXTRN   RM_ALLOC:NEAR           ; Allocate pages
                EXTRN   INIT_MEMORY:NEAR        ; Initialize memory management
                EXTRN   INIT_HIMEM:NEAR         ; Update high memory table
                EXTRN   CLEANUP_MEMORY:NEAR     ; Cleanup memory management
                EXTRN   INIT_MEM_PHYS:NEAR      ; Convert lists to PM format

INIT_CODE       ENDS

              ENDIF
