;
; PAGING.INC -- Manage page tables
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

;
; Bits in a page table entry which are used by the processor
;
PAGE_PRESENT    =        01H            ; Page present
PAGE_WRITE      =        02H            ; Page writable
PAGE_USER       =        04H            ; Not a supervisor page
PAGE_ACCESSED   =        20H            ; Page has been accessed
PAGE_DIRTY      =        40H            ; Page is dirty
;
; Bits in a page table entry which are available for OS use
;
PAGE_LOCKED     =       200H            ; Page not swappable
PAGE_ALLOC      =       400H            ; Memory allocated
PAGE_UNUSED_2   =       800H            ; Unused

;
; PAGE_CLEAR is used with AND for clearing the address and the
; present, dirty, and accessed bits of a page table entry
;
PAGE_PDA        =       PAGE_PRESENT OR PAGE_DIRTY OR PAGE_ACCESSED
PAGE_CLEAR      =       0FFFH AND NOT PAGE_PDA

;
; User ID for supervisor (used in swapper table entries)
;
PIDX_SV         =       0FFH            ; Supervisor page


              IFNDEF __PAGING

SV_DATA         SEGMENT

                EXTRN   LIN_START:DWORD         ; First usable linear address
                EXTRN   PAGE_DIR_PHYS:DWORD     ; Phys. addr. of page directory
                EXTRN   PAGE_DIR_SEG:WORD       ; Segment of page directory
                EXTRN   PAGING:BYTE             ; Paging enabled
                EXTRN   PAGE_TAB0_SEG:WORD      ; Seg. of 0th page table (VCPI)

SV_DATA         ENDS


SV_CODE         SEGMENT

                EXTRN   INIT_PAGES:NEAR, ALLOC_PAGES:NEAR, INIT_PAGE_BMAP:NEAR
                EXTRN   SET_PAGES:NEAR, CLEAR_TLB:NEAR, INIT_LIN_MAP:NEAR
                EXTRN   FREE_LIN:NEAR, FREE_PAGES:NEAR, GET_FREE_PAGE:NEAR
                EXTRN   FREE_AVAIL:NEAR, CLEAR_DIRTY:NEAR
                EXTRN   MAYBE_MAP_PAGE:NEAR

SV_CODE         ENDS


INIT_CODE       SEGMENT

                EXTRN   INIT_PAGING:NEAR, MAP_PAGES:NEAR, RM_TO_PHYS:NEAR

INIT_CODE       ENDS

              ENDIF
