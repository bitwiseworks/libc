;
; MEMORY.ASM -- Manage memory
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

__MEMORY        =       1
                INCLUDE EMX.INC
                INCLUDE SWAPPER.INC
                INCLUDE VCPI.INC
                INCLUDE XMS.INC
                INCLUDE MEMORY.INC
                INCLUDE SEGMENTS.INC
                INCLUDE PAGING.INC
                INCLUDE RPRINT.INC
                INCLUDE OPRINT.INC
                INCLUDE RMINT.INC
                INCLUDE OPTIONS.INC
                INCLUDE MISC.INC

                PUBLIC  LOMEM_HEAD_PHYS, HIMEM_HEAD_PHYS, DISABLE_EXT_MEM
                PUBLIC  HIMEM_COUNT, HIMEM_TAB, VDISK_FLAG, TAVAIL
                PUBLIC  DISABLE_LOW_MEM
                PUBLIC  PM_ALLOC, PM_ALLOC_NOSWAP, INIT_HIMEM, PM_AVAIL
                PUBLIC  RM_ALLOC, INIT_MEMORY, CLEANUP_MEMORY, INIT_MEM_PHYS


DEBUG_EXT_MEM   =       FALSE
DEBUG_LOW_MEM   =       FALSE

;
; This data structure should fit into one paragraph (16 bytes). The first
; paragraph of any block allocated from DOS memory will contain this
; structure. The fields are used for allocating pages in real mode (or
; virtual mode). When no more pages are required, the blocks will either
; be converted into a linked list of pages for allocating in protected mode
; or shrunk to the size used to save low memory, allowing executing DOS
; programs.
;
RM_BLOCK_HEADER STRUCT
RBH_NEXT        DW      ?               ; Segment address of next block
RBH_PAGE        DW      ?               ; Segment of next free page
RBH_MAX         DW      ?               ; Maximum number of pages
RBH_FREE        DW      ?               ; Number of free pages
RM_BLOCK_HEADER ENDS

                .ERRE   SIZE RM_BLOCK_HEADER LE 16

SV_DATA         SEGMENT

LOMEM_HEAD_PHYS LABEL   DWORD           ; Physical address of LOMEM_HEAD
                DW      LOMEM_HEAD, 0

HIMEM_HEAD_PHYS LABEL   DWORD           ; Physical address of HIMEM_HEAD
                DW      HIMEM_HEAD, 0

                DALIGN  16              ; Real-mode offset is 0!
LOMEM_HEAD      MEMORY_BLOCK    <>

                DALIGN  16              ; Real-mode offset is 0!
HIMEM_HEAD      MEMORY_BLOCK    <>

                DALIGN  16              ; Real-mode offset is 0!
BLOCKS_HEAD     DW      NULL_RM         ; Head of linked list of memory blocks

EXT_1K_BLOCKS   DW      ?               ; Extended memory (1K blocks)
DISABLE_EXT_MEM DB      FALSE           ; Don't use memory above 1M: off
DISABLE_LOW_MEM DB      FALSE           ; Don't use memory below 1M: off
INT15_HOOKED    DB      FALSE
VDISK_FLAG      DB      FALSE           ; VDISK 3.3 detected: no

                DALIGN  2
HIMEM_COUNT     DW      0               ; Total number of entries
HIMEM_TAB       HIMEM_ENTRY HIMEM_MAX DUP (<>)

HIMEM_AVAIL_COUNT       DW      0       ; Entries remaining for allocation
HIMEM_AVAIL_START       DW      0       ; Pointer to first available entry

EXT_START       EQU     HIMEM_TAB[0].HM_ADDR
EXT_SIZE        EQU     HIMEM_TAB[0].HM_SIZE

                DALIGN  2
BLOCK_MOVE_GDT  DQ      0               ; null
BM_GDT          DD      ?, ?            ; GDT
BM_SRC          DD      ?, ?            ; source
BM_DST          DD      ?, ?            ; destination
BM_BIOS         DD      ?, ?            ; BIOS code segment
BM_STACK        DD      ?, ?            ; stack

                DALIGN  2
VDISK_BOOT      DB      3 DUP (?)       ; jump
VDISK_NAME      DB      8 DUP (?)       ; name
                DB      19 DUP (?)      ; BPB
VDISK_NEXT      DW      ?               ; First free byte (1K increments)
                DALIGN  2               ; Make size even (move words!)
VDISK_WORDS     =       (THIS BYTE - VDISK_BOOT)/2

VDISK_ANY       DB      "VDISK"
VDISK_33_LABEL  DB      "VDISK  V3.3"
VDISK_33_BOOT   DB      "VDISK3.3"

VDISK_DRIVER    STRUC
VD_NEXT         DD      ?
VD_ADDR         DW      ?
VD_STRATEGY     DW      ?
VD_INTERRUPT    DW      ?
VD_DEV_NAME     DB      8 DUP (?)       ; First byte: number of block devices
VD_VOL_NAME     DB      11 DUP (?)      ; Volume label
VD_VOL_ATTR     DB      ?
VD_VOL_RES      DB      10 DUP (?)
VD_VOL_TIME     DW      ?
VD_VOL_DATE     DW      ?
VD_AVAIL_LO     DW      ?
VD_AVAIL_HI     DB      ?
VDISK_DRIVER    ENDS

                DALIGN  4
TAVAIL          DD      ?                       ; Number of available pages

;
; Messages
;
$VDISK_ERROR    DB      "Unsupported VDISK.SYS version", CR, LF, 0

              IF DEBUG_MEM
$MEMI1          DB      "Address=", 0
$MEMI2          DB      " size=", 0
              ENDIF

              IF DEBUG_EXT_MEM
$NO_VDISK       DB      "No VDISK found", CR, LF, 0
$VDISK1         DB      "VDISK found, next=", 0
$VDISK2         DB      "INT 19H: next=", 0
$EXT_MEM        DB      " 1K pages of extended memory", CR, LF, 0
$EXT_START      DB      "Use extended memory at ", 0
$EXT_SIZE       DB      "Size=", 0
              ENDIF

              IF DEBUG_LOW_MEM
$RM_ALLOC_1     DB      "RM_ALLOC(", 0
$RM_ALLOC_2     DB      ")=", 0
$INIT_MEM_1     DB      "INIT_MEM: seg=", 0
$INIT_MEM_2     DB      ", len=", 0
              ENDIF

SV_DATA         ENDS


SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; Allocate 4096 bytes of memory (either high or low memory, swap out a
; page if required)
;
; Out:  EAX     Physical address or NULL_PHYS if out of memory
;

                ASSUME  DS:SV_DATA
                TALIGN  4
PM_ALLOC        PROC    NEAR
                CALL    PM_ALLOC_NOSWAP         ; Try without swapping
                CMP     EAX, NULL_PHYS          ; Memory found?
                JNE     SHORT PA_RET            ; Yes -> done
                CALL    SWAP_OUT                ; Try to swap out
PA_RET:         RET
PM_ALLOC        ENDP

;
; Allocate 4096 bytes of memory (either high or low memory, without
; swapping)
;
; Out:  EAX     Physical address or NULL_PHYS if out of memory
;
                ASSUME  DS:SV_DATA
                TALIGN  4
PM_ALLOC_NOSWAP PROC    NEAR
                PUSH    ES
                PUSH    EBX
                PUSH    ECX
                PUSH    EDX
                PUSH    ESI
PAN_RESTART:
              IF DEBUG_MEM
                CALL    MEM_INFO
              ENDIF
                CMP     DISABLE_EXT_MEM, FALSE  ; Extended memory disabled?
                JNE     SHORT PAN_LOW           ; Yes -> use low memory
;
; Try XMS before EMS as we have already acquired all available XMS
; memory and EMS memory is allocated on demand.  That leaves more
; (EMS) memory to DOS programs.
;
PAN_HIGH:       MOV     EBX, HIMEM_HEAD_PHYS
                CALL    PA_LOOK_LIST            ; Look for high memory in list
                CMP     EAX, NULL_PHYS          ; Page found?
                JNE     SHORT PAN_DONE          ; Yes -> done
                CALL    PA_LOOK_TAB             ; Look for high memory in table
                CMP     EAX, NULL_PHYS          ; Page found?
                JNE     SHORT PAN_DONE          ; Yes -> done
;
; No XMS page available, try VCPI
;
PAN_EMS:        CMP     VCPI_FLAG, FALSE        ; Running under VCPI?
                JE      SHORT PAN_LOW           ; No  -> use low memory
                CALL    VCPI_ALLOC              ; Allocate one page
                CMP     EAX, NULL_PHYS          ; Success?
                JNE     SHORT PAN_DONE          ; Yes -> done
;
; Neither XMS nor EMS memory is available, try low memory
;
PAN_LOW:        MOV     EBX, LOMEM_HEAD_PHYS
                CALL    PA_LOOK_LIST            ; Look for low memory
                CMP     EAX, NULL_PHYS          ; Page found ?
                JNE     SHORT PAN_DONE          ; Yes -> done
                CALL    GET_FREE_PAGE           ; Look in free page bitmap
;
; Map the page if not already done.  If the allocated page was used
; for its page table, we have to start over.
;
PAN_DONE:       CMP     EAX, NULL_PHYS          ; Out of memory?
                JE      PAN_RET                 ; Yes -> done
                MOV     EBX, EAX
                ADD     EBX, G_PHYS_BASE        ; EBX := linear address
                JC      SHORT PAN_ERROR         ; Must not happen
                CALL    MAYBE_MAP_PAGE
                CMP     EAX, NULL_PHYS          ; Used for page table?
                JE      PAN_RESTART             ; Yes -> repeat
PAN_RET:
              IF DEBUG_MEM
                CALL    MEM_INFO
              ENDIF
                POP     ESI
                POP     EDX
                POP     ECX
                POP     EBX
                POP     ES
                RET

PAN_ERROR:      INT     3
                JMP     SHORT $
PM_ALLOC_NOSWAP ENDP



;
; Get page from table of blocks
;
; Out   EAX     Physical address (NULL_PHYS: no page found)
;
                ASSUME  DS:SV_DATA
PA_LOOK_TAB     PROC    NEAR
                PUSH    ECX                     ; Save ECX
                MOV     EAX, NULL_PHYS
                MOV     CX, HIMEM_AVAIL_COUNT   ; Get table info
                JCXZ    PALT_RET                ; Table used up -> done
                MOV     SI, HIMEM_AVAIL_START
                ASSUME  SI:PTR HIMEM_ENTRY
PALT_1:         CMP     [SI].HM_AVAIL, 0
                JNE     PALT_2                  ; Hit!
                ADD     SI, SIZE HIMEM_ENTRY
                LOOP    PALT_1
                JMP     SHORT PALT_3

PALT_2:         MOV     EAX, [SI].HM_USED
                ADD     [SI].HM_USED, 4096
                SUB     [SI].HM_AVAIL, 4096
PALT_3:         MOV     HIMEM_AVAIL_COUNT, CX   ; Update table info
                MOV     HIMEM_AVAIL_START, SI
PALT_RET:       POP     ECX                     ; Restore ECX
                RET
                ASSUME  SI:NOTHING
PA_LOOK_TAB     ENDP


;
; Get page from list of pages
;
; In:   EBX     Physical address of chain head
;
; Out   EAX     Physical address (NULL_PHYS: no page found)
;       ES      G_PHYS_SEL
;
                ASSUME  EBX:NEAR32 PTR MEMORY_BLOCK
PA_LOOK_LIST    PROC    NEAR
                PUSH    ECX                      ; Save ECX
                MOV     AX, G_PHYS_SEL           ; Use phyiscal addresses
                MOV     ES, AX
                MOV     EAX, ES:[EBX].NEXT_BLOCK ; Pointer to first page
                CMP     EAX, NULL_PHYS           ; Chain empty?
                JE      SHORT PALL_RET           ; Yes -> no page found
                ASSUME  EAX:NEAR32 PTR MEMORY_BLOCK
                MOV     ECX, ES:[EAX].NEXT_BLOCK ; Remove this page from
                MOV     ES:[EBX].NEXT_BLOCK, ECX ; the chain
                ASSUME  EAX:NOTHING
PALL_RET:       POP     ECX                      ; Restore ECX
                RET
                ASSUME  EBX:NOTHING
PA_LOOK_LIST    ENDP


;
; Available memory w/o swapping
;
; Out:  EAX     Number of available pages
;
                ASSUME  DS:SV_DATA
PM_AVAIL        PROC    NEAR
                MOV     TAVAIL, 0               ; No pages available
                CMP     DISABLE_EXT_MEM, FALSE  ; Extended memory disabled?
                JNE     SHORT PV_LOW            ; Yes -> low memory only
                CMP     VCPI_FLAG, FALSE        ; Running under VCPI?
                JE      SHORT PV_HIGH           ; No  -> skip
                CALL    VCPI_AVAIL              ; VCPI memory available
PV_HIGH:        MOV     EBX, HIMEM_HEAD_PHYS
                CALL    PV_LOOK
PV_LOW:         MOV     EBX, LOMEM_HEAD_PHYS
                CALL    PV_LOOK
                CALL    FREE_AVAIL
                MOV     EAX, TAVAIL
                RET
PM_AVAIL        ENDP


;
; Add the number of available pages in a list of pages to TAVAIL
;
; In:   EBX     Physical address of chain head
;
                ASSUME  DS:SV_DATA
                ASSUME  EBX:NEAR32 PTR MEMORY_BLOCK
PV_LOOK         PROC    NEAR
                PUSH    ES                      ; Save ES
                PUSH    EAX                     ; Save EAX
                MOV     AX, G_PHYS_SEL          ; Use physical addresses
                MOV     ES, AX
                TALIGN  2
PV_LOOK_NEXT:   MOV     EBX, ES:[EBX].NEXT_BLOCK ; Get address of next page
                CMP     EBX, NULL_PHYS          ; End of chain?
                JE      SHORT PV_LOOK_RET       ; Yes -> done
                INC     TAVAIL                  ; Increment number of pages
                JMP     SHORT PV_LOOK_NEXT      ; Repeat
PV_LOOK_RET:    POP     EAX                     ; Restore EAX
                POP     ES                      ; Restore ES
                RET
                ASSUME  EBX:NOTHING
PV_LOOK         ENDP

                ASSUME  DS:NOTHING

             IF DEBUG_MEM

MEM_INFO        PROC    NEAR
                PUSH    DS
                PUSH    FS
                PUSHAD
                MOV     AX, G_SV_DATA_SEL
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                CALL    OCRLF
                MOV     CX, HIMEM_AVAIL_COUNT
                MOV     DI, HIMEM_AVAIL_START
                CALL    MEM_INFO_TAB
                MOV     AX, G_PHYS_SEL
                MOV     FS, AX
                MOV     EDI, LOMEM_HEAD_PHYS
                CALL    MEM_INFO_LIST
                MOV     EDI, HIMEM_HEAD_PHYS
                CALL    MEM_INFO_LIST
              IF DEBUG_MEM_WAIT
                MOV     AH, 01H
                INT     21H
              ENDIF
                POPAD
                NOP                             ; Avoid 386 bug
                POP     FS
                POP     DS
                RET
MEM_INFO        ENDP

                ASSUME  EDI:NEAR32 PTR MEMORY_BLOCK
MEM_INFO_LIST   PROC    NEAR
MIL_1:          LEA     EDX, $MEMI1
                CALL    OTEXT
                MOV     EAX, EDI
                CALL    ODWORD
                CALL    OCRLF
                MOV     EDI, FS:[EDI].NEXT_BLOCK
                CMP     EDI, NULL_PHYS
                JNE     MIL_1
                RET
MEM_INFO_LIST   ENDP
                ASSUME  EDI:NOTHING

                ASSUME  DI:PTR HIMEM_ENTRY
MEM_INFO_TAB    PROC    NEAR
                JCXZ    MIT_RET
MIT_1:          CMP     [DI].HM_AVAIL, 0
                JE      MIT_2
                LEA     EDX, $MEMI1
                CALL    OTEXT
                MOV     EAX, [DI].HM_USED
                CALL    ODWORD
                LEA     EDX, $MEMI2
                CALL    OTEXT
                MOV     EAX, [DI].HM_AVAIL
                CALL    ODWORD
                CALL    OCRLF
MIT_2:          ADD     DI, SIZE HIMEM_ENTRY
                LOOP    MIT_1
MIT_RET:        RET
MEM_INFO_TAB    ENDP
                ASSUME  DI:NOTHING

              ENDIF


SV_CODE         ENDS



INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

OLD_INT15       LABEL   DWORD
OLD_INT15_OFF   DW      ?
OLD_INT15_SEG   DW      ?


;
; Allocate low memory
;
; In:   AX      Number of 4K pages to allocate
;
; Out:  AX      Real-mode segment or NULL_RM if no pages found
;
; Note: Must *not* be called after calling INIT_MEM_PHYS.
;
                ASSUME  DS:SV_DATA
                TALIGN  2
RM_ALLOC        PROC    NEAR
                PUSH    ES
                PUSH    CX
                PUSH    DX
                MOV     DX, BLOCKS_HEAD         ; Head of list
                MOV     CX, AX
                TALIGN  2
RA_LOOP:        CMP     DX, NULL_RM             ; End of list?
                JE      SHORT RA_FAIL           ; Yes -> fail
                MOV     ES, DX
                MOV     DX, (RM_BLOCK_HEADER PTR ES:[0]).RBH_NEXT
                                                ; Segment of next block
                CMP     CX, (RM_BLOCK_HEADER PTR ES:[0]).RBH_FREE
                                                ; Are there enough free pages?
                JA      SHORT RA_LOOP           ; No  -> next block
                SUB     (RM_BLOCK_HEADER PTR ES:[0]).RBH_FREE, AX
                                                ; Adjust number of free pages
                SHL     AX, 12 - 4              ; Convert to paragraphs
                MOV     DX, (RM_BLOCK_HEADER PTR ES:[0]).RBH_PAGE
                                                ; Get segment address of page
                ADD     (RM_BLOCK_HEADER PTR ES:[0]).RBH_PAGE, AX
                                                ; Adjust page address
                MOV     AX, DX                  ; Return segment in AX
                JMP     SHORT RA_RET
;
; Not enough pages found -> return NULL_RM
;
RA_FAIL:        MOV     AX, NULL_RM             ; Fail
RA_RET:         
              IF DEBUG_LOW_MEM
                LEA     DX, $RM_ALLOC_1
                CALL    RTEXT
                PUSH    AX
                MOV     AX, CX
                CALL    RWORD
                POP     AX
                LEA     DX, $RM_ALLOC_2
                CALL    RTEXT
                CALL    RWORD
                CALL    RCRLF
              ENDIF
                POP     DX
                POP     CX
                POP     ES
                RET
RM_ALLOC        ENDP


;
; Initialize memory allocation
;
                ASSUME  DS:SV_DATA

INIT_MEMORY     PROC    NEAR
                PUSH    ES                      ; Save ES
                LEA     AX, BLOCKS_HEAD
                SHR     AX, 4                   ; ALIGN 16!
                ADD     AX, SV_DATA
                MOV     ES, AX                  ; Previous block
;
; Allocate blocks of memory and add to list of low memory blocks
;
IM_ALLOC:       MOV     BX, 0FFFFH              ; Try to allocate 0FFFFH
                MOV     AH, 48H                 ; paragraphs (always fails)
                INT     21H
                CMP     BX, (16+4096)/16        ; Not enough memory available?
                JB      SHORT IM_CONT           ; Yes -> done
                MOV     AH, 48H                 ; Allocate largest block
                INT     21H
                JC      SHORT IM_CONT           ; Out of memory -> done
              IF DEBUG_LOW_MEM
                LEA     DX, $INIT_MEM_1
                CALL    RTEXT
                CALL    RWORD
                LEA     DX, $INIT_MEM_2
                CALL    RTEXT
                PUSH    AX
                MOV     AX, BX
                CALL    RWORD
                CALL    RCRLF
                POP     AX
              ENDIF
                MOV     DX, AX                  ; Save the segment
;
; Compute linear address and size of block, hide header
;
                MOVZX   EBX, BX
                SHL     EBX, 4                  ; Size of block (bytes)
                SUB     EBX, 16                 ; Header
                MOVZX   EAX, AX
                SHL     EAX, 4                  ; Linear address of block
                ADD     EAX, 16                 ; Header
;
; Align start and end to page size
;
                TEST    EAX, 0FFFH              ; Page aligned?
                JZ      SHORT IM_ALIGNED
                ADD     EBX, EAX                ; EBX := end of block
                OR      EAX, 0FFFH
                INC     EAX                     ; Now EAX is page aligned
                SUB     EBX, EAX                ; EBX := new block size
IM_ALIGNED:     AND     EBX, NOT 0FFFH          ; Align end of block
                JZ      SHORT IM_ALLOC          ; Too small -> next block
;
; Add block to list
;
                MOV     (RM_BLOCK_HEADER PTR ES:[0]).RBH_NEXT, DX
                                                ; Add header to list
                MOV     ES, DX                  ; Now initialize the fields
                SHR     EAX, 4                  ; Compute first segment
                MOV     (RM_BLOCK_HEADER PTR ES:[0]).RBH_PAGE, AX
                                                ; Next (first, now) free page
                SHR     EBX, 12                 ; Number of pages
                MOV     (RM_BLOCK_HEADER PTR ES:[0]).RBH_MAX, BX
                MOV     (RM_BLOCK_HEADER PTR ES:[0]).RBH_FREE, BX
                JMP     IM_ALLOC                ; Allocate another block

IM_CONT:        MOV     (RM_BLOCK_HEADER PTR ES:[0]).RBH_NEXT, NULL_RM
                                                ; End of chain
                CMP     VCPI_FLAG, FALSE        ; VCPI active?
                JNE     IM_RET                  ; Yes -> himem managed by VCPI
                CMP     XMS_FLAG, FALSE         ; XMS active?
                JNE     IM_RET                  ; Yes -> himem managed by XMS
;
; Neither XMS nor VCPI. Do extended memory the hard way
;
                CMP     DISABLE_EXT_MEM, FALSE  ; Extended memory disabled?
                JNE     IM_RET                  ; Yes -> skip
                CMP     MACHINE, MACH_FMR70     ; Fujitsu FMR70?
                JNE     SHORT IM_HIMEM1         ; No  ->
;
; Fujitsu FMR70
;
                MOV     AX, FMR70_MEM_MAX
                JMP     SHORT IM_SET_EXT

IM_HIMEM1:      CMP     MACHINE, MACH_PC98      ; NEC PC-98?
                JNE     SHORT IM_PC             ; No  -> COMPAQ / IBM
                XOR     AX, AX
                MOV     ES, AX
                XCHG    AL, BYTE PTR ES:[401H]  ; Get memory size (128KB)
                SHL     AX, 7                   ; Multiply by 128KB
                JMP     SHORT IM_SET_EXT
;
; First check whether we have extended memory at all
;
IM_PC:          MOV     AH, 88H
                INT     15H
IM_SET_EXT:     MOV     EXT_1K_BLOCKS, AX
              IF DEBUG_EXT_MEM
                CALL    RWORD
                LEA     DX, $EXT_MEM
                CALL    RTEXT
              ENDIF
                OR      AX, AX
                JZ      IM_RET
                MOVZX   EAX, AX
                SHL     EAX, 10                 ; Multiply by 1K
                MOV     EXT_SIZE, EAX
                MOV     EXT_START, 100000H      ; 1M
;
; Look for VDISK in extended memory
;
                CMP     MACHINE, MACH_PC        ; COMPAQ / IBM?
                JE      SHORT VDISK_PC          ; Yes -> look for VDISK
                CMP     MACHINE, MACH_INBOARD   ; Inboard 386/PC?
                JNE     NO_VDISK                ; No  -> skip
VDISK_PC:       MOV     ESI, 100000H            ; 1M
                LEA     DI, VDISK_BOOT
                MOV     CX, VDISK_WORDS
                CALL    GET_EXT
                MOV_ES_DS
                LEA     SI, VDISK_NAME
                LEA     DI, VDISK_ANY
                MOV     CX, SIZE VDISK_ANY
                CLD
                REPE    CMPS VDISK_NAME, ES:VDISK_ANY
                JNE     NO_VDISK
                LEA     SI, VDISK_NAME
                LEA     DI, VDISK_33_BOOT
                MOV     CX, SIZE VDISK_33_BOOT
                CLD
                REPE    CMPS VDISK_NAME, ES:VDISK_33_BOOT
                JNE     SHORT VDISK_ERROR
                MOVZX   EAX, VDISK_NEXT
                SHL     EAX, 10                 ; Multiply by 1K
                MOV     EXT_START, EAX          ; Set start address
              IF DEBUG_EXT_MEM
                LEA     DX, $VDISK1
                CALL    RTEXT
                MOV     AL, BYTE PTR EXT_START[2]
                CALL    RBYTE
                MOV     AX, WORD PTR EXT_START[0]
                CALL    RWORD
                CALL    RCRLF
              ENDIF
;
; Look for VDISK (INT 19H)
;
                MOV     AH, 35H
                MOV     AL, 19H                 ; Get INT 19H
                INT     21H
                XOR     BX, BX                  ; Use offset 0
                MOV     DI, BX
                ADD     DI, VDISK_DRIVER.VD_VOL_NAME
                LEA     SI, VDISK_33_LABEL
                MOV     CX, SIZE VDISK_33_LABEL
                CLD
                REPE    CMPS VDISK_33_LABEL, ES:[DI]
                JE      SHORT VDISK_1
VDISK_ERROR:    LEA     DX, $VDISK_ERROR
                CALL    RTEXT
                MOV     AL, 0FFH
                JMP     EXIT

                ASSUME  BX:PTR VDISK_DRIVER
VDISK_1:      IF DEBUG_EXT_MEM
                LEA     DX, $VDISK2
                CALL    RTEXT
                MOV     AL, ES:[BX].VD_AVAIL_HI
                CALL    RBYTE
                MOV     AX, ES:[BX].VD_AVAIL_LO
                CALL    RWORD
                CALL    RCRLF
              ENDIF
                MOV     VDISK_FLAG, NOT FALSE
                MOVZX   EAX, ES:[BX].VD_AVAIL_HI
                SHL     EAX, 16
                MOV     AX, ES:[BX].VD_AVAIL_LO ; Compute start address
                ASSUME  BX:NOTHING
                CMP     EAX, EXT_START          ; Inconsistent?
                JBE     SHORT VDISK_2           ; No ->
                MOV     EXT_START, EAX          ; Be defensive
VDISK_2:        MOV     EAX, EXT_START
                SUB     EAX, 100000H            ; Bytes used by VDISK.SYS
                JBE     SHORT VDISK_ERROR       ; Be cautious
                SUB     EXT_SIZE, EAX           ; Reduce memory size
                JB      SHORT VDISK_ERROR       ; Be cautious
                JZ      SHORT IM_RET            ; No extended memory left
                JMP     SHORT USE_EXT_MEM
;
; Allocate extended memory by hooking INT 15H
;
NO_VDISK:
              IF DEBUG_EXT_MEM
                LEA     DX, $NO_VDISK
                CALL    RTEXT
              ENDIF
USE_EXT_MEM:
              IF DEBUG_EXT_MEM
                LEA     DX, $EXT_START
                CALL    RTEXT
                MOV     AX, WORD PTR EXT_START[2]
                CALL    RWORD
                MOV     AX, WORD PTR EXT_START[0]
                CALL    RWORD
                CALL    RCRLF
                LEA     DX, $EXT_SIZE
                CALL    RTEXT
                MOV     AX, WORD PTR EXT_SIZE[2]
                CALL    RWORD
                MOV     AX, WORD PTR EXT_SIZE[0]
                CALL    RWORD
                CALL    RCRLF
              ENDIF
                MOV     HIMEM_COUNT, 1          ; One himem block
                CMP     MACHINE, MACH_PC        ; COMPAQ / IBM?
                JE      SHORT IM_HOOK           ; Yes -> hook INT 15H
                CMP     MACHINE, MACH_INBOARD   ; Inboard 386/PC?
                JNE     SHORT IM_RET            ; No  -> don't hook INT 15H
IM_HOOK:        MOV     AL, 15H
                LEA     DX, MY_INT15
                CALL    SET_RM_INT
                MOV     OLD_INT15_SEG, AX
                MOV     OLD_INT15_OFF, DX
                MOV     INT15_HOOKED, NOT FALSE
IM_RET:         POP     ES                      ; Restore ES
                RET
INIT_MEMORY     ENDP


;
; Convert the blocks allocated from DOS memory (linked list of blocks) to
; protected-mode format (linked list of pages using physical addresses).
;
INIT_MEM_PHYS   PROC    NEAR
                CMP     DISABLE_LOW_MEM, FALSE  ; Use low memory?
                JNE     SHORT IMP_FREE          ; No  -> give back to DOS
                LEA     BX, LOMEM_HEAD
                SHR     BX, 4
                ADD     BX, SEG LOMEM_HEAD      ; BX:0 is destination
                MOV     AX, NULL_RM
                XCHG    AX, BLOCKS_HEAD
IMP_OUTER:      CMP     AX, NULL_RM             ; End of list?
                JE      SHORT IMP_OUTER_END     ; Yes -> done
                MOV     ES, AX
                PUSH    (RM_BLOCK_HEADER PTR ES:[0]).RBH_NEXT
                                                ; Next block
                MOV     CX, (RM_BLOCK_HEADER PTR ES:[0]).RBH_FREE
                                                ; Number of free pages
                JCXZ    IMP_OUTER_NEXT
                MOV     AX, (RM_BLOCK_HEADER PTR ES:[0]).RBH_PAGE
                                                ; Segment of first free page
IMP_INNER:      PUSH    AX
                XOR     DX, DX
                CALL    RM_TO_PHYS              ; Convert AX:DX to phys. addr
                MOV     ES, BX
                MOV     (MEMORY_BLOCK PTR ES:[0]).NEXT_BLOCK, EAX
                POP     AX
                MOV     BX, AX
                ADD     AX, 1000H / 10H         ; Next page in block
                LOOP    IMP_INNER
IMP_OUTER_NEXT: POP     AX
                JMP     SHORT IMP_OUTER

IMP_OUTER_END:  MOV     ES, BX
                MOV     (MEMORY_BLOCK PTR ES:[0]).NEXT_BLOCK, NULL_PHYS
                                                ; End of chain
                RET

;
; Give back unused low memory to DOS
;
IMP_FREE:       MOV     LOMEM_HEAD.NEXT_BLOCK, NULL_PHYS ; Don't use low mem
                MOV     AX, NULL_RM
                XCHG    AX, BLOCKS_HEAD         ; Head of list
IMP_FREE_LOOP:  CMP     AX, NULL_RM             ; End of list?
                JE      SHORT IMP_FREE_END      ; Yes -> done
                MOV     ES, AX
                PUSH    (RM_BLOCK_HEADER PTR ES:[0]).RBH_NEXT
                                                ; Pointer to next block
                MOV     AX, (RM_BLOCK_HEADER PTR ES:[0]).RBH_MAX
                OR      AX, AX                  ; Any pages in block?
                JZ      SHORT IMP_FREE_FREE     ; No -> free block
                MOV     AX, (RM_BLOCK_HEADER PTR ES:[0]).RBH_FREE
                OR      AX, AX                  ; Are there free pages?
                JZ      IMP_OUTER_NEXT          ; No free pages -> next block
                CMP     AX, (RM_BLOCK_HEADER PTR ES:[0]).RBH_MAX
                                                ; Any pages used?
                JNE     SHORT IMP_FREE_SET      ; Yes -> shrink block
IMP_FREE_FREE:  MOV     AH, 49H
                INT     21H                     ; Free allocated memory
                JMP     SHORT IMP_FREE_NEXT

IMP_FREE_SET:   MOV     BX, (RM_BLOCK_HEADER PTR ES:[0]).RBH_PAGE
                                                ; Address of next free page
                MOV     AX, ES
                SUB     BX, AX                  ; Number of paragraphs
                MOV     AH, 4AH
                INT     21H                     ; Set block
IMP_FREE_NEXT:  POP     AX                      ; Get pointer to next block
                JMP     SHORT IMP_FREE_LOOP     ; Repeat until end reached

IMP_FREE_END:   RET

INIT_MEM_PHYS   ENDP

;
; Update the table of extended memory blocks.  Up to emx 0.9d, this procedure
; put all the pages into a linked list (HIMEM_HEAD_PHYS).  Now, we can no
; longer do this, as the memory is not yet accessible (the page tables
; do not yet exist as high memory cannot be allocated -- chicken and egg!).
; Instead, we allocate memory directly from the table of extended memory
; blocks.
;
INIT_HIMEM      PROC    NEAR
                MOV     CX, HIMEM_COUNT         ; Number of XMS blocks
                LEA     SI, HIMEM_TAB           ; Table of XMS blocks
                ASSUME  SI:PTR HIMEM_ENTRY
                MOV     HIMEM_AVAIL_COUNT, CX
                MOV     HIMEM_AVAIL_START, SI
                JCXZ    IHM_END                 ; None -> done
IHM_1:          MOV     EBX, [SI].HM_ADDR       ; Get address of block
                MOV     EDX, [SI].HM_SIZE       ; Get size of block
;
; Align block
;
                TEST    EBX, 0FFFH              ; Page aligned?
                JZ      SHORT IHM_2
                ADD     EDX, EBX                ; EDX := end of block
                OR      EBX, 0FFFH
                INC     EBX                     ; Now EBX is page aligned
                SUB     EDX, EBX                ; EDX := new block size
IHM_2:          AND     EDX, NOT 0FFFH          ; Align end of block
;
; Initialize HM_USED and HM_AVAIL
;
                MOV     [SI].HM_USED, EBX
                MOV     [SI].HM_AVAIL, EDX
;
; End of loop
;
                ADD     SI, SIZE HIMEM_ENTRY    ; Move to next block
                LOOP    IHM_1                   ; Repeat for all blocks
                ASSUME  SI:NOTHING
IHM_END:        RET
INIT_HIMEM      ENDP

;
; INT 15H handler. Request 88H (get extended memory size) returns 0,
; all other requests are passed on to the original INT 15H handler.
;
                TALIGN  2
MY_INT15        PROC    FAR
                CMP     AH, 88H                 ; Get extended memory size
                JNE     SHORT MY_INT15_OLD
                XOR     AX, AX                  ; No extended memory
                IRET
                TALIGN  2
MY_INT15_OLD:   JMP     OLD_INT15
MY_INT15        ENDP

;
; Remove INT 15H hook.
;
CLEANUP_MEMORY  PROC    NEAR
                CMP     INT15_HOOKED, FALSE
                JE      SHORT CUM_RET
                MOV     AL, 15H
                MOV     BX, OLD_INT15_SEG
                MOV     DX, OLD_INT15_OFF
                CALL    RESTORE_RM_INT
CUM_RET:        RET
CLEANUP_MEMORY  ENDP


;
; Read extended memory (real mode!)
;
; Supports lower 16MB only!
;
; In:   ESI     Source address (>=1MB)
;       DS:DI   Destination address
;       CX      Number of words
;

MAKE_DES        MACRO   @E
                MOV     WORD PTR @E[0], 0FFFFH  ; Segment limit
                MOV     WORD PTR @E[2], AX      ; Segment base 0..15
                SHR     EAX, 16
                MOV     BYTE PTR @E[4], AL      ; Segment base 16..23
                MOV     BYTE PTR @E[5], 93H
                MOV     WORD PTR @E[6], 0       ; It's a 16 bit segment
                ENDM

GET_EXT         PROC    NEAR
                PUSH    ES
                PUSH    EAX
                PUSH    EBX
                PUSH    ESI
                XOR     EAX, EAX
                MOV     BM_GDT[0], EAX
                MOV     BM_GDT[4], EAX
                MOV     BM_BIOS[0], EAX
                MOV     BM_BIOS[4], EAX
                MOV     BM_STACK[0], EAX
                MOV     BM_STACK[4], EAX
                MOVZX   EDI, DI
                MOV     AX, DS                  ; EAX=0!
                SHL     EAX, 4
                ADD     EAX, EDI                ; Destination address
                MAKE_DES BM_DST
                MOV     EAX, ESI
                MAKE_DES BM_SRC
                MOV     AH, 87H
                MOV_ES_DS
                LEA     SI, BLOCK_MOVE_GDT
                INT     15H
                POP     ESI
                POP     EBX
                POP     EAX
                POP     ES
                RET
GET_EXT         ENDP

INIT_CODE       ENDS


                END
