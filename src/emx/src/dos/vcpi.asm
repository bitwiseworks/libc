;
; VCPI.ASM -- Virtual Control Program Interface
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

                INCLUDE EMX.INC
                INCLUDE TABLES.INC
                INCLUDE PMINT.INC
                INCLUDE PAGING.INC
                INCLUDE SEGMENTS.INC
                INCLUDE RPRINT.INC
                INCLUDE MEMORY.INC
                INCLUDE OPRINT.INC
                INCLUDE MISC.INC

                PUBLIC  VCPI_FLAG, DV_FLAG
                PUBLIC  V2P_LIN, V2P_CR3, V2P_IDTR, V2P_GDTR
                PUBLIC  START_VCPI, INT_BACK_VCPI, INT_VCPI
                PUBLIC  VCPI_ALLOC, VCPI_AVAIL
                PUBLIC  INIT_VCPI, CHECK_VCPI, GET_INT_MAP, SET_INT_MAP
                PUBLIC  CHECK_VM, CLEANUP_VCPI, VCPI_LIN_TO_PHYS

SV_DATA         SEGMENT

                TALIGN  2
V2P_LIN         LABEL   DWORD
                DW      VCPI_TO_PROT, 0

                TALIGN  2
VCPI_TO_PROT    LABEL   DWORD
V2P_CR3         DD      ?
V2P_GDTR        LABEL   DWORD
                DW      GDT_PTR, 0
V2P_IDTR        LABEL   DWORD
                DW      IDT_PTR, 0
V2P_LDTR        DW      G_LDT_SEL
V2P_TR          DW      G_TSS_SEL
V2P_EIP         DD      ?
V2P_CS          DW      G_SV_CODE_SEL

;
; We have to remember all VCPI pages we've allocated, as we have to
; deallocate them when terminating. This is done by maintaining a
; bitmap of pages allocated by calling the VCPI server. The first bit
; refers to physical address 0. Currently, only one page is used for
; the bitmap, therefore only 128 MB of memory are supported. This bitmap
; must be in low memory, as deallocation is done in real mode.
;
VCPI_BITMAP_PHYS DD     ?               ; Points to the bitmap
VCPI_BITMAP_SEG DW      0               ; Real-mode segment, zero: not alloc.


EMM_PAGE        DW      0
EMM_PAGE_FLAG   DB      FALSE           ; EMM_PAGE allocated

VCPI_FLAG       DB      FALSE
DV_FLAG         DB      FALSE

;
; Names of devices implemented by expanded memory managers
;
EMS_FNAME       DB      "EMMXXXX0", 0           ; EMS enabled
NOEMS_FNAME1    DB      "EMMQXXX0", 0           ; EMS disabled
NOEMS_FNAME2    DB      "$MMXXXX0",0            ; EMS disabled

$NO_VCPI        DB      "Virtual mode not supported without VCPI", CR, LF, 0
$DESQVIEW       DB      "DESQview without expanded memory emulator not supported", CR, LF, 0
$VA_BUG         DB      "Page allocated more than once", CR, LF, 0
$RTP_FAILURE    DB      "VCPI function 6 error", CR, LF, 0

                DALIGN  4

VCPI_ENTRY      LABEL   FWORD
VCPI_OFF        DD      ?
                DW      G_SERVER_SEL


SV_DATA         ENDS



SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; Switch from protected mode to virtual mode
;
                ASSUME  DS:NOTHING
                TALIGN  4
INT_VCPI:       MOV     AX, G_ZERO_SEL
                MOV     DS, AX
                ASSUME  DS:NOTHING
                XOR     EAX, EAX
                MOV     AX, SV_DATA
                PUSH    EAX                     ; GS
                PUSH    EAX                     ; FS
                PUSH    EAX                     ; DS
                PUSH    EAX                     ; ES
                MOV     AX, RM_STACK
                PUSH    EAX                     ; SS
                LEA     AX, RM_TOS
                PUSH    EAX                     ; ESP
                XOR     EAX, EAX
                PUSH    EAX                     ; EFLAGS
                MOV     AX, INIT_CODE
                PUSH    EAX                     ; CS
                LEA     EAX, V2V_CONT
                PUSH    EAX                     ; EIP
                XOR     EAX, EAX
                PUSH    EAX
                PUSH    EAX
                MOV     AX, G_SV_DATA_SEL
                MOV     ES, AX
                ASSUME  ES:SV_DATA
                MOV     AX, 0DE0CH
                CLI
                JMP     VCPI_ENTRY

                ASSUME  ES:NOTHING

;
; Allocate one page
;
; Out:  EAX     Physical address (or NULL_PHYS if failed)
;
                ASSUME  DS:SV_DATA
VCPI_ALLOC      PROC    NEAR
                PUSH    ES
                PUSH    EDX
                PUSH    EBX
                MOV     AX, G_PHYS_SEL
                MOV     ES, AX
VA_RETRY:       MOV     AX, 0DE04H              ; Allocate a 4K page
                CALL    VCPI_ENTRY
                OR      AH, AH                  ; Failure?
                JNZ     SHORT VA_ERROR          ; Yes -> return 0
                MOV     EAX, EDX
                SHR     EAX, 12                 ; Physical page number
                CMP     EAX, 4096*8             ; Number of bits exceeded?
                JAE     SHORT VA_RETRY          ; Yes -> retry, don't free
                MOV     EBX, VCPI_BITMAP_PHYS
                BTS     DWORD PTR ES:[EBX], EAX ; Test/set bit
                JC      SHORT VA_BUG            ; Impossible
                MOV     EAX, EDX
                JMP     SHORT VA_RET

VA_ERROR:       MOV     EAX, NULL_PHYS          ; Yes -> return NULL_PHYS
VA_RET:         POP     EBX
                POP     EDX
                POP     ES
                RET

VA_BUG:         LEA     EDX, $VA_BUG
                CALL    OTEXT                   ; Display error message
                MOV     AX, 4CFFH               ; Quit
                INT     21H
                JMP     SHORT $                 ; Never reached
VCPI_ALLOC      ENDP


;
; Available VCPI memory
;
; Amount of available memory will be added to TAVAIL.
;
                ASSUME  DS:SV_DATA
VCPI_AVAIL      PROC    NEAR
                PUSH    EAX
                PUSH    EDX
                MOV     AX, 0DE03H              ; Get number of free 4K pages
                CALL    VCPI_ENTRY
                OR      AH, AH                  ; Failure?
                JNZ     SHORT VAV_RET           ; Yes -> no memory
                ADD     TAVAIL, EDX             ; Add to sum
VAV_RET:        POP     EDX
                POP     EAX
                RET
VCPI_AVAIL      ENDP


SV_CODE         ENDS


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Check presence of VCPI server
;
                ASSUME  DS:SV_DATA
CHECK_VCPI      PROC    NEAR
                MOV     EMM_PAGE_FLAG, FALSE
;
; First, check the EMS interrupt vector
;
                MOV     AX, 3567H               ; Get interrupt vector 67H
                INT     21H
                MOV     AX, ES
                OR      AX, BX                  ; Is the vector 0:0 ?
                JZ      SHORT CV_END            ; Yes -> no VCPI
;
; Check for an EMM with EMS enabled
;
                LEA     DX, EMS_FNAME
                CALL    CHECK_EMM               ; EMS present?
                JC      SHORT EMM_DISABLED      ; No  -> try disabled EMS
;
; Check EMM status
;
                MOV     AH, 40H                 ; Get EMM status
                INT     67H
                CMP     AH, 0                   ; Successful?
                JNE     SHORT CV_END            ; No  -> no VCPI
;
; Try to allocate a page.  Ignore failure
;
                MOV     BX, 1
                MOV     AH, 43H                 ; Allocate an EMS page
                INT     67H
                CMP     AH, 0
                JNE     SHORT EMS_1
                MOV     EMM_PAGE, DX
                MOV     EMM_PAGE_FLAG, NOT FALSE
EMS_1:          JMP     SHORT EMM_OK

;
; Check for an EMM with EMS disabled (using undocumented "features")
;
EMM_DISABLED:   LEA     DX, NOEMS_FNAME1
                CALL    CHECK_EMM
                JNC     SHORT EMM_OK            ; EMM present
                LEA     DX, NOEMS_FNAME2
                CALL    CHECK_EMM
                JC      SHORT CV_END            ; No EMM present -> no VCPI
;
; There is an EMM with EMS enabled or disabled.  Check for VCPI
;
EMM_OK:         MOV     AX, 0DE00H              ; VCPI presence detection
                INT     67H
                CMP     AH, 0                   ; VCPI present?
                JNE     SHORT CV_END            ; No ->
;
; VCPI server present.  Don't deallocate the page to prevent the VCPI
; server from switching back to real mode
;
                MOV     VCPI_FLAG, NOT FALSE
                JMP     SHORT CV_RET

;
; Dellocate the EMS page if we have allocated one
;
CV_END:         CMP     EMM_PAGE_FLAG, FALSE
                JE      SHORT CV_RET
                MOV     EMM_PAGE_FLAG, FALSE
                MOV     DX, EMM_PAGE
                MOV     AH, 45H
                INT     67H
CV_RET:         RET
CHECK_VCPI      ENDP


;
; Check for the presence of an expanded memory manager
;
; In:   DX      Pointer to device name
;
; Out:  NC      EMM present
;
                EVEN
CHECK_EMM       PROC    NEAR
                MOV     AX, 3D00H               ; Open
                INT     21H
                JC      SHORT CE_RET
;
; A file or device with that name exists.  Check for a device
;
                MOV     BX, AX                  ; Handle
                MOV     AX, 4400H               ; IOCTL: get device data
                INT     21H
                JC      SHORT CE_FAIL           ; Failure -> no EMM
                TEST    DL, 80H                 ; Device?
                JZ      SHORT CE_FAIL           ; No -> no EMM
                MOV     AX, 4407H               ; IOCTL: check output status
                INT     21H
                JC      SHORT CE_FAIL           ; Failure -> no EMM
                CMP     AL, 0FFH                ; Ready?
                JNE     SHORT CE_FAIL           ; No  -> no EMM
                MOV     AH, 3EH                 ; Close
                INT     21H
                CLC
CE_RET:         RET

CE_FAIL:        MOV     AH, 3EH                 ; Close
                INT     21H
                STC
                RET
CHECK_EMM       ENDP


;
; Fatal exit for some cases not handled without VCPI (virtual mode, DESQview)
;
                EVEN
CHECK_VM        PROC    NEAR
                CMP     VCPI_FLAG, FALSE
                JNE     SHORT CVM_RET
                .386P
                SMSW    AX                      ; Get machine status word
                .386
                TEST    AX, 0001H               ; Protected (virtual) mode?
                JZ      SHORT NO_VCPI_1         ; No -> continue
                LEA     DX, $NO_VCPI            ; We've a problem...
VCPI_ERROR:     CALL    RTEXT
                MOV     AL, 0FFH
                JMP     EXIT

;
; No VCPI: DESQview not supported
;
NO_VCPI_1:      CMP     DV_FLAG, FALSE          ; Running under DESQview?
                JE      SHORT CVM_RET           ; No -> ok
                LEA     DX, $DESQVIEW           ; Not supported
                JMP     SHORT VCPI_ERROR

CVM_RET:        RET
CHECK_VM        ENDP



;
; Free all pages allocated via VCPI
;
CLEANUP_VCPI    PROC    NEAR
                CMP     VCPI_FLAG, FALSE        ; VCPI active?
                JE      SHORT CUV_RET           ; No -> nothing to do
                MOV     AX, VCPI_BITMAP_SEG     ; Get segment of bitmap
                OR      AX, AX                  ; Bitmap allocated?
                JZ      SHORT CUV_5             ; No -> nothing to do
                MOV     ES, AX
                XOR     DI, DI                  ; Bitmap pointer
                XOR     EDX, EDX                ; Physical address
CUV_1:          CMP     DI, 4096                ; End of bitmap?
                JAE     SHORT CUV_5             ; Yes -> done
                CMP     DWORD PTR ES:[DI], 0    ; Any bits in this DWORD?
                JNZ     SHORT CUV_2             ; Yes -> deallocate pages
                ADD     DI, 4                   ; Next DWORD
                ADD     EDX, 32*4096            ; Adjust physical address
                JMP     SHORT CUV_1             ; Repeat

CUV_2:          MOV     CX, 32                  ; Examine 32 bits
                XOR     ESI, ESI
                XCHG    ESI, ES:[DI]            ; Fetch DWORD, set to zero
CUV_3:          SHR     ESI, 1                  ; Bit set?
                JNC     SHORT CUV_4             ; No  -> skip
                MOV     AX, 0DE05H              ; Free a 4K page
                INT     67H
CUV_4:          ADD     EDX, 4096               ; Adjust physical address
                LOOP    CUV_3                   ; Repeat for all 32 bits
                ADD     DI, 4                   ; Next DWORD
                JMP     SHORT CUV_1

;
; Deallocate the EMS page allocated by the VCPI presence check
;
CUV_5:          CMP     EMM_PAGE_FLAG, FALSE
                JE      SHORT CUV_RET
                MOV     EMM_PAGE_FLAG, FALSE
                MOV     DX, EMM_PAGE
                MOV     AH, 45H
                INT     67H
CUV_RET:        RET
CLEANUP_VCPI    ENDP


;
; VCPI specific initializations, setup page tables, allocate bitmap.
;
                ASSUME  DS:SV_DATA
INIT_VCPI       PROC    NEAR
                MOV     AX, 1
                CALL    RM_ALLOC                ; Allocate 0th page table
                CMP     AX, NULL_RM             ; Out of memory?
                JE      RM_OUT_OF_MEM           ; Yes -> abort
                MOV     PAGE_TAB0_SEG, AX       ; Save address
                MOV     ES, AX
                XOR     DI, DI
                MOV     ECX, 1024
                XOR     EAX, EAX
                CLD
                REP     STOS DWORD PTR ES:[DI]
                XOR     DI, DI                  ; ES:DI --> 0th page table
                LEA     SI, G_VCPI_DESC         ; DS:SI --> GDT entries
                MOV     AX, 0DE01H              ; Get protected mode interface
                INT     67H
;
; RM_TO_PHYS works from now onwards.
;
                MOV     VCPI_OFF, EBX
                MOV     LIN_START, 1 SHL 22     ; 1st page table
;
; Put page table 0 into page directory
;
                MOV     AX, PAGE_TAB0_SEG
                XOR     DX, DX
                CALL    RM_TO_PHYS
                OR      EAX, 7
                MOV     FS, PAGE_DIR_SEG
                MOV     FS:[0], EAX
;
; Create linear address space for G_PHYS_SEL
;
; The VCPI server initializes parts of page table 0 (0..4M), at least the
; first 1M. We must not change the page table entries initialized by the VCPI
; server. But the `Allocate a 4K Page' call of the VCPI server may return a
; page with a physical address in this range. Now we have a problem: How
; shall we access those pages, as we cannot use identity mapping?
; Solution: We remap the entire physical memory for use with G_PHYS_SEL.
; The part of the 0th page table initialized by the VCPI server will be
; copied.
;
; Important: LIN_START must point to an unused page table (or no
;            swapper info will be created). This is done by making
;            LIN_START point to the beginning of the next page table.
;
                MOV     AX, 0DE02H              ; Get maximum physical address
                INT     67H
                ADD     EDX, 1000H              ; First free physical address
                ADD     EDX, (1 SHL 22)-1       ; Round to next page table
                AND     EDX, NOT ((1 SHL 22)-1)
                MOV     EBP, 0                  ; Physical address = 0
                MOV     FS, PAGE_DIR_SEG        ; Page directory
                MOV     SI, 4                   ; Page table 1
                MOV     EAX, LIN_START          ; First free linear address
                PUSH    EAX                     ; Save for G_PHYS_SEL base
                ADD     LIN_START, EDX          ; First free linear address
                MOV     ECX, EDX                ; Compute number of pages
                SHR     ECX, 22                 ; of physical memory
                PUSH    EDX
                CALL    MAP_PAGES               ; Initialize page tables
IVCPI_1:        POP     ECX
                POP     EAX                     ; Base address of G_PHYS_SEL
                MOV     G_PHYS_BASE, EAX
                LEA     DI, G_PHYS_DESC
                CALL    RM_SEG_BASE
                CALL    RM_SEG_SIZE
;
; Allocate and zero the VCPI allocation bitmap
;
                MOV     AX, 1
                CALL    RM_ALLOC                ; Allocate bitmap
                CMP     AX, NULL_RM             ; Out of memory?
                JE      RM_OUT_OF_MEM           ; Yes -> abort
                MOV     VCPI_BITMAP_SEG, AX     ; Save segment
                PUSH    AX
                XOR     DX, DX                  ; Offset = 0
                CALL    RM_TO_PHYS              ; Compute physical address
                MOV     VCPI_BITMAP_PHYS, EAX   ; Save physical address
                POP     ES                      ; Restore segment
                XOR     DI, DI                  ; Zero the bitmap
                XOR     AL, AL
                MOV     CX, 4096
                CLD
                REP     STOS BYTE PTR ES:[DI]
                RET
INIT_VCPI       ENDP


;
; Start protected mode
;
                ASSUME  DS:SV_DATA
START_VCPI      PROC    NEAR
                MOV     V2P_EIP, OFFSET PROT1   ; Protected mode entry point
                MOV     ESI, V2P_LIN            ; Table of registers
                MOV     AX, 0DE0CH              ; Switch to protected mode
                CLI                             ; Required by VCPI specs.
                INT     67H                     ; Call VCPI server
                JMP     SHORT $                 ; Hardly ever reached (note 1)
;
; Note 1: INT 67H doesn't work when single-stepping into the interrupt
;         routine due to virtual mode and changed IDTR.
;
START_VCPI      ENDP


;
; Return from virtual mode to protected-mode interrupt handler
;
                ASSUME  DS:SV_DATA
                TALIGN  4
INT_BACK_VCPI   PROC    NEAR
                MOV     V2P_EIP, OFFSET V2P_CONT
                MOV     ESI, V2P_LIN
                MOV     AX, 0DE0CH
                CLI
                INT     67H
                JMP     SHORT $                 ; Hardly ever reached (note 1)
INT_BACK_VCPI   ENDP


;
; Get interrupt vector mappings
;
                ASSUME  DS:NOTHING
GET_INT_MAP     PROC    NEAR
                MOV     AX, 0DE0AH
                INT     67H                     ; Get interrupt vector mappings
                RET
GET_INT_MAP     ENDP


;
; Inform VCPI server about interrupt vector mappings
;
                ASSUME  DS:NOTHING
SET_INT_MAP     PROC    NEAR
                MOV     AX, 0DE0BH
                INT     67H
                RET
SET_INT_MAP     ENDP


;
; Convert linear address in first Megabyte to physical address
;
; In:   EAX     Linear address (less than 1M)
;
; Out:  EAX     Physical address
;
                ASSUME  DS:SV_DATA
VCPI_LIN_TO_PHYS PROC   NEAR
                PUSH    CX                      ; Save CX
                PUSH    EDX                     ; Save EDX
                PUSH    EAX                     ; Save linear address
                SHR     EAX, 12                 ; Compute page number
                MOV     CX, AX
                MOV     AX, 0DE06H              ; Get physical address of 4K
                INT     67H                     ; page in first Megabyte
                OR      AH, AH                  ; Success?
                JNZ     SHORT RTP_FAILURE       ; No  -> stop (cannot happen)
                POP     EAX                     ; Restore linear address
                AND     EAX, 0FFFH              ; Keep offset into page
                ADD     EAX, EDX                ; Add physical address
                POP     EDX                     ; Restore EDX
                POP     CX                      ; Restore CX
                RET

RTP_FAILURE:    LEA     DX, $RTP_FAILURE        ; Error message
                CALL    RTEXT
                MOV     AL, 0FFH                ; Return code = 255
                JMP     EXIT                    ; Quit

VCPI_LIN_TO_PHYS ENDP


INIT_CODE       ENDS

                END
