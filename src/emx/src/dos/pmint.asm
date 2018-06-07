;
; PMINT.ASM -- Handle interrupts
;
; Copyright (c) 1991-1996 by Eberhard Mattes
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
; Handle interrupts in protected mode (hardware and software interrupts).
; Includes DOS interface.
;

__PMINT         =       1
                INCLUDE EMX.INC
                INCLUDE TABLES.INC
                INCLUDE VPRINT.INC
                INCLUDE OPRINT.INC
                INCLUDE PAGING.INC
                INCLUDE SYSCALL.INC
                INCLUDE EXTAPI.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE PROFIL.INC
                INCLUDE VCPI.INC
                INCLUDE PMINT.INC
                INCLUDE EXCEPT.INC
                INCLUDE RPRINT.INC
                INCLUDE A20.INC
                INCLUDE DEBUG.INC
                INCLUDE SEGMENTS.INC
                INCLUDE OPTIONS.INC
                INCLUDE MISC.INC
                INCLUDE CORE.INC
                INCLUDE RMINT.INC
                INCLUDE FILEIO.INC
                INCLUDE UTILS.INC
                INCLUDE TERMIO.INC
                INCLUDE STAT.INC
                INCLUDE ERRORS.INC

                PUBLIC  IRQ0_ADD, IRQ8_ADD
                PUBLIC  INTERRUPT, PMINT_21H, PMINT_10H, PMINT_11H
                PUBLIC  PMINT_14H, PMINT_16H, PMINT_17H, PMINT_31H, PMINT_33H
                PUBLIC  DOSF_ERROR, BREAK_AFTER_IRET, NMI, INT_RM
                PUBLIC  BUF_SEG, BUF1_SEG, BUF2_SEG, BUF_SEL, RM_CR0, PM_CR0
                PUBLIC  V2P_CONT, V2V_CONT, INIT_BUFFER
                PUBLIC  PMINT0_TAB, PMINT8_TAB, DBCS_LEAD_TAB
                PUBLIC  DOS_INT         ; put into map file for debugging

SER_FLAG        =       FALSE                   ; Don't use serial interface
DEBUG_DOSDUMP   =       FALSE

SV_DATA         SEGMENT

PMINT0_TAB      LABEL   WORD
                IRP     X,<0,1,2,3,4,5,6,7>
                WORD    OFFSET SV_CODE:PMINT&X
                ENDM
PMINT8_TAB      LABEL   WORD
                IRP     X,<8,9,10,11,12,13,14,15>
                WORD    OFFSET SV_CODE:PMINT&X
                ENDM

PMHWINT_LSS     LABEL   FWORD
PMHWINT_ESP     DWORD   ?
PMHWINT_SS      WORD    ?
;
; This variable points to the interrupt stack frame of protected mode
; (in the protected-mode stack SV_STACK). It is used for accessing the
; stack frame in real mode. Be sure to save this variable when passing
; signals from real mode to protected mode!
;
FRAME_PTR       WORD    ?

;
; Before switching back to real/virtual mode, we restore the floating point
; related bits of CR0 saved at the beginning. We can save the bits only
; in protected mode, as CR0 isn't readable in virtual mode. After switching
; to protected mode, we restore the floating point related bits of CR0
; from PM_CR0.
;
                DALIGN  4
RM_CR0          DWORD   ?                       ; Real-mode CR0 (AND CR0_FPU)
PM_CR0          DWORD   ?                       ; Prot-mode CR0 (AND CR0_FPU)

;
; Table of conversions
;

DF_INVALID      =       00H     ; syscall function not supported
DF_NULL         =       01H     ; No parameters to be converted
DF_WRITE        =       02H     ; Input:  src=DS:EDX, length=ECX -> DS:DX, CX
DF_PATH         =       03H     ; Input:  pathname=DS:EDX -> DS:DX
DF_TEXT         =       04H     ; Input:  text=DS:EDX ($ terminated) -> DS:DX
DF_READ         =       05H     ; Output: dst=DS:DX, length=AX (CY=0 only) -> DS:EDX
DF_SEEK         =       06H     ; Input: EDX -> CX:DX; output: DX:AX -> EAX (CY=0)
DF_TMP          =       07H     ; Input/output: DS:DX (0 term.) + 13 bytes
DF_INPUT        =       08H     ; Input/output: DS:DX, length=DX[0]+2
DF_MOVE         =       09H     ; Input: DS:EDX -> DS:DX, DS:EDI -> ES:DI file names
DF_DUP2         =       0AH     ; Force duplicate file handle
DF_CLOSE        =       0BH     ; Close file handle
DF_IOCTL        =       0CH     ; IOCTL, use additional table
DF_GETCWD       =       0DH     ; Output: DS:SI -> DS:ESI (64 bytes)
DF_EXIT         =       0EH     ; Exit
DF_FFIRST       =       0FH     ; Find first (DS:EDX, DTA DS:ESI)
DF_FNEXT        =       10H     ; Find next (DTA DS:ESI)
DF_VERSION      =       11H     ; Return DOS version (and emx)
DF_EXEC         =       12H     ; Load and execute program
DF_RETCODE      =       13H     ; Get return code of child process
DF_STR_BP       =       14H     ; Input: EBP null-terminated string -> ES:BP
DF_VESA         =       15H     ; VESA BIOS (10H, AH=4FH), use additional table
DF_VESAINFO     =       16H     ; VESA function 00H (return 256 bytes)
DF_VESAMODE     =       17H     ; VESA function 01H (return 256 bytes)
DF_MOU_GCURSOR  =       18H     ; Mouse function 09H (in: ES:DX -> 64 bytes)
DF_MOU_LGCURSOR =       19H     ; Mouse function 12H (in: ES:DX -> var. bytes)
DF_COUNTRY      =       1AH     ; Output: EDX->DS:DX (34 bytes) unless DX=-1
DF_LOCK         =       1BH     ; Input: EDX->CX:DX, EDI->SI:DI
DF_TRUENAME     =       1CH     ; DOS function 60H
DF_PATH_SI      =       1DH     ; Input:  pathname=DS:ESI -> DS:SI
DF_DUP          =       1EH     ; Duplicate file handle
DF_VIO11        =       1FH     ; INT 10H AH=11H, use additional table (AL)
DF_VIO12        =       20H     ; INT 10H AH=12H, use additional table (BL)
DF_LAST         =       20H
DF_SPARSE_END   =      0FFH     ; End of sparse table

;
; The following flag bits are used for INT 21H only.  They also apply
; to the INT 21H IOCTL calls.
;
DFE             =     8000H     ; CY indicates error, convert to errno
DFM_STDIN       =     4000H     ; Map stdin before invoking INT 21H
DFM_STDOUT      =     2000H     ; Map stdout before invoking INT 21H
DFM_STDIO       =     DFM_STDIN OR DFM_STDOUT

DFH_IN          =     0100H     ; BX contains file handle (in)
DFH_OUT         =     0200H     ; New file handle returned in EAX (out, CY=0)
DFH_IN_OUT      =     DFH_IN OR DFH_OUT

;
; Get conversion flag from a table
;
; In:   BL      Function number
;
; Out:  AX      Conversion flag
;       BX      Modified
;
TBGET           MACRO   TABLE
                TABLE&_GET
                ENDM

;
; Start a dense table
;
; In:   TABLE   Name of the table
;       MAXFUN  Highest function number
;
DENSE_TABLE     MACRO   TABLE, MAXFUN
TABLE           LABEL   WORD
DENSE_NEXT      =       0
DENSE_MAXFUN    =       MAXFUN
TBENTRY         MACRO   FUNC, FLAG
                DENSE_ENTRY FUNC, FLAG
                ENDM
TBEND           MACRO
                DENSE_END
                ENDM
TABLE&_MAXFUN   =       MAXFUN
TABLE&_GET      MACRO
                DENSE_GET TABLE
                ENDM
                ENDM

;
; Add an entry to a dense table
;
DENSE_ENTRY     MACRO   FUNC, FLAG
                .ERRE   FUNC GE DENSE_NEXT
              IF FUNC NE DENSE_NEXT
                WORD    FUNC-DENSE_NEXT DUP (DF_INVALID)
DENSE_NEXT      =       FUNC
              ENDIF
                WORD    FLAG
DENSE_NEXT      =       DENSE_NEXT+1
                ENDM

;
; End a dense table
;
DENSE_END       MACRO
              IF DENSE_NEXT NE DENSE_MAXFUN + 1
                DENSE_ENTRY DENSE_MAXFUN, <DF_INVALID>
              ENDIF
                ENDM

;
; Get conversion flag from a dense table
;
; In:   BL      Function number
;
; Out:  AX      Conversion flag
;       BX      Modified
;
DENSE_GET       MACRO   TABLE
              IF TABLE&_MAXFUN NE 0FFH
                CMP     BL, TABLE&_MAXFUN
                JA      D_INVALID
              ENDIF
                XOR     BH, BH
                SHL     BX, 1
                MOV     AX, TABLE[BX]
                ENDM

;
; Start a sparse table
;
; In:   TABLE   Name of the table
;
SPARSE_TABLE    MACRO   TABLE
TABLE           LABEL   BYTE
TBENTRY         MACRO   FUNC, FLAG
                SPARSE_ENTRY FUNC, FLAG
                ENDM
TBEND           MACRO
                SPARSE_END
                ENDM
TABLE&_GET      MACRO
                SPARSE_GET TABLE
                ENDM
                ENDM

;
; Add an entry to a sparse table
;
SPARSE_ENTRY    MACRO   FUNC, FLAG
                BYTE    FUNC
                WORD    FLAG
                ENDM

;
; End a sparse table
;
SPARSE_END      MACRO
                SPARSE_ENTRY 00H, DF_SPARSE_END
                ENDM

;
; Get conversion flag from a sparse table
;
; In:   BL      Function number
;
; Out:  AX      Conversion flag
;       BX      Modified
;
SPARSE_GET      MACRO   TABLE
                LOCAL   LOOP1, DONE
                MOV     AL, BL
                LEA     BX, TABLE - 3
LOOP1:          ADD     BX, 3
                CMP     BYTE PTR [BX+1], DF_SPARSE_END
                JE      D_INVALID
                CMP     [BX+0], AL
                JNE     LOOP1
                MOV     AX, [BX+1]
                ENDM


        DENSE_TABLE     DOS_FUNCS, 6CH
        TBENTRY 01H, <DF_NULL OR DFM_STDIO>          ; Read keyboard and echo
        TBENTRY 02H, <DF_NULL OR DFM_STDOUT>         ; Display character
        TBENTRY 03H, <DF_NULL>                       ; Auxiliary input
        TBENTRY 04H, <DF_NULL>                       ; Auxiliary output
        TBENTRY 05H, <DF_NULL>                       ; Print character
        TBENTRY 06H, <DF_NULL OR DFM_STDIO>          ; Direct console i/o
        TBENTRY 07H, <DF_NULL OR DFM_STDIN>          ; Direct console input
        TBENTRY 08H, <DF_NULL OR DFM_STDIN>          ; Read keyboard
        TBENTRY 09H, <DF_TEXT OR DFM_STDOUT>         ; Display string
        TBENTRY 0AH, <DF_INPUT OR DFM_STDIO>         ; Buffered keyboard input
        TBENTRY 0BH, <DF_NULL OR DFM_STDIN>          ; Check keyboard status
        TBENTRY 0CH, <DF_NULL OR DFM_STDIO>          ; Flush keyboard buffer
        TBENTRY 0DH, <DF_NULL>                       ; Reset disk
        TBENTRY 0EH, <DF_NULL>                       ; Select disk
        TBENTRY 19H, <DF_NULL>                       ; Get current disk
        TBENTRY 2AH, <DF_NULL>                       ; Get date
        TBENTRY 2BH, <DF_NULL>                       ; Set date
        TBENTRY 2CH, <DF_NULL>                       ; Get time
        TBENTRY 2DH, <DF_NULL>                       ; Set time
        TBENTRY 2EH, <DF_NULL>                       ; Set/reset verify flag
        TBENTRY 30H, <DF_VERSION>                    ; Get DOS version number
        TBENTRY 33H, <DF_NULL>                       ; Control-C check
        TBENTRY 36H, <DF_NULL>                       ; Get disk free space
        TBENTRY 37H, <DF_NULL>                       ; Get/set switch character
        TBENTRY 38H, <DFE OR DF_COUNTRY>             ; Get/set country data
        TBENTRY 39H, <DFE OR DF_PATH>                ; Create directory
        TBENTRY 3AH, <DFE OR DF_PATH>                ; Remove directory
        TBENTRY 3BH, <DFE OR DF_PATH>                ; Change current directory
        TBENTRY 3CH, <DFE OR DF_PATH OR DFH_OUT>     ; Create handle
        TBENTRY 3DH, <DFE OR DF_PATH OR DFH_OUT>     ; Open handle
        TBENTRY 3EH, <DFE OR DF_CLOSE>               ; Close handle
        TBENTRY 3FH, <DFE OR DF_READ OR DFH_IN>      ; Read handle
        TBENTRY 40H, <DFE OR DF_WRITE OR DFH_IN>     ; Write handle
        TBENTRY 41H, <DFE OR DF_PATH>                ; Delete directory entry
        TBENTRY 42H, <DFE OR DF_SEEK OR DFH_IN>      ; Move file pointer
        TBENTRY 43H, <DFE OR DF_PATH>                ; Get/set file attributes
        TBENTRY 44H, <DF_IOCTL>                      ; IOCTL
        TBENTRY 45H, <DF_DUP>                        ; Duplicate file handle
        TBENTRY 46H, <DF_DUP2>                       ; Force duplicate handle
        TBENTRY 47H, <DFE OR DF_GETCWD>              ; Get current directory
        TBENTRY 4BH, <DFE OR DF_EXEC>                ; Load and execute program
        TBENTRY 4CH, <DF_EXIT>                       ; End process
        TBENTRY 4DH, <DF_RETCODE>                    ; Get rc of child process
        TBENTRY 4EH, <DFE OR DF_FFIRST>              ; Find first file
        TBENTRY 4FH, <DFE OR DF_FNEXT>               ; Find next file
        TBENTRY 54H, <DF_NULL>                       ; Get verify state
        TBENTRY 56H, <DFE OR DF_MOVE>                ; Change directory entry
        TBENTRY 57H, <DFE OR DF_NULL OR DFH_IN>      ; Time/date of file
        TBENTRY 58H, <DFE OR DF_NULL>                ; Get/set alloc strat
        TBENTRY 59H, <DF_NULL>                       ; Get extended error
        TBENTRY 5AH, <DFE OR DF_TMP OR DFH_OUT>      ; Create temporary file
        TBENTRY 5BH, <DFE OR DF_PATH OR DFH_OUT>     ; Create new file
        TBENTRY 5CH, <DFE OR DF_LOCK OR DFH_IN>      ; Lock
        TBENTRY 60H, <DFE OR DF_TRUENAME>            ; Canonicalize path
        TBENTRY 66H, <DFE OR DF_NULL>                ; Get/set global cp table
        TBENTRY 67H, <DFE OR DF_NULL>                ; Set handle count
        TBENTRY 68H, <DFE OR DF_NULL OR DFH_IN>      ; Commit file
        TBENTRY 6AH, <DFE OR DF_NULL OR DFH_IN>      ; Commit file
        TBENTRY 6CH, <DFE OR DF_PATH_SI OR DFH_OUT>  ; Extended open/create
        TBEND

        DENSE_TABLE     IOCTL_FUNCS, 0BH
        TBENTRY 00H, <DFE OR DF_NULL OR DFH_IN>      ; Get device data
        TBENTRY 01H, <DFE OR DF_NULL OR DFH_IN>      ; Set device data
        TBENTRY 02H, <DFE OR DF_WRITE OR DFH_IN>     ; Send control data (char)
        TBENTRY 03H, <DFE OR DF_READ OR DFH_IN>      ; Receive ctl data (char)
        TBENTRY 04H, <DFE OR DF_WRITE>               ; Send ctl data (block)
        TBENTRY 05H, <DFE OR DF_READ>                ; Receive ctl data (block)
        TBENTRY 06H, <DFE OR DF_NULL OR DFH_IN>      ; Check input status
        TBENTRY 07H, <DFE OR DF_NULL OR DFH_IN>      ; Check output status
        TBENTRY 08H, <DFE OR DF_NULL>                ; Is changable
        TBENTRY 09H, <DFE OR DF_NULL>                ; Is redirected block
        TBENTRY 0AH, <DFE OR DF_NULL OR DFH_IN>      ; Is redirected handle
        TBENTRY 0BH, <DFE OR DF_NULL>                ; Retry
        TBEND

        DENSE_TABLE     VIO_FUNCS, 0FFH
        TBENTRY 00H, <DF_NULL>                       ; Set video mode
        TBENTRY 01H, <DF_NULL>                       ; Set cursor shape
        TBENTRY 02H, <DF_NULL>                       ; Set cursor position
        TBENTRY 03H, <DF_NULL>                       ; Set cursor pos and size
        TBENTRY 04H, <DF_NULL>                       ; Read light pen position
        TBENTRY 05H, <DF_NULL>                       ; Select display page
        TBENTRY 06H, <DF_NULL>                       ; Scroll up
        TBENTRY 07H, <DF_NULL>                       ; Scroll down
        TBENTRY 08H, <DF_NULL>                       ; Read char and attribute
        TBENTRY 09H, <DF_NULL>                       ; Write char and attribute
        TBENTRY 0AH, <DF_NULL>                       ; Write character
        TBENTRY 0BH, <DF_NULL>                       ; Set palette
        TBENTRY 0CH, <DF_NULL>                       ; Write graphics pixel
        TBENTRY 0DH, <DF_NULL>                       ; Read graphics pixel
        TBENTRY 0EH, <DF_NULL>                       ; Teletype output
        TBENTRY 0FH, <DF_NULL>                       ; Get current video mode
        TBENTRY 10H, <DF_NULL>                       ; Palette (AL != 02H,
                                                     ; 09H, 12H, 17H)
        TBENTRY 11H, <DF_VIO11>                      ; Font
        TBENTRY 12H, <DF_VIO12>                      ; Miscellaneous stuff
        TBENTRY 13H, <DF_STR_BP>                     ; Write string
        TBENTRY 14H, <DF_INVALID>                    ; LCD
        TBENTRY 15H, <DF_INVALID>                    ; Get parameters
        TBENTRY 1AH, <DF_NULL>                       ; Display combination
        TBENTRY 1BH, <DF_INVALID>                    ; State information
        TBENTRY 1CH, <DF_INVALID>                    ; Save/restore state
        TBENTRY 1FH, <DF_INVALID>                    ; XGA DMQS
        TBENTRY 30H, <DF_INVALID>                    ; 3270 stuff
        TBENTRY 40H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 41H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 42H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 43H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 44H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 45H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 46H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 47H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 48H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 49H, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 4AH, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 4BH, <DF_NULL>                       ; Hercules GRAFIX / FRIEZE
        TBENTRY 4CH, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 4DH, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 4EH, <DF_NULL>                       ; Hercules GRAFIX
        TBENTRY 4FH, <DF_VESA>                       ; VESA
        TBENTRY 6FH, <DF_NULL>                       ; Various functions
        TBENTRY 0F0H, <DF_NULL>                      ; EGA register interface
        TBENTRY 0F1H, <DF_NULL>                      ; EGA register interface
        TBENTRY 0F6H, <DF_NULL>                      ; EGA register interface
        TBEND

        DENSE_TABLE     VESA_FUNCS, 08H
        TBENTRY 00H, <DF_VESAINFO>                   ; Get SVGA info
        TBENTRY 01H, <DF_VESAMODE>                   ; Get SVGA mode info
        TBENTRY 02H, <DF_NULL>                       ; Set mode
        TBENTRY 03H, <DF_NULL>                       ; Get mode
        TBENTRY 04H, <DF_INVALID>                    ; Save/restore state
        TBENTRY 05H, <DF_NULL>                       ; Memory control
        TBENTRY 06H, <DF_NULL>                       ; Scan line length
        TBENTRY 07H, <DF_NULL>                       ; Display start
        TBENTRY 08H, <DF_NULL>                       ; DAC palette control
        TBEND

        DENSE_TABLE     VIO11_FUNCS, 24H
        TBENTRY 01H, <DF_NULL>                       ; Load ROM patterns 8x14
        TBENTRY 02H, <DF_NULL>                       ; Load ROM patterns 8x8
        TBENTRY 03H, <DF_NULL>                       ; Set block specifier
        TBENTRY 04H, <DF_NULL>                       ; Load ROM charset 8x14
        TBENTRY 11H, <DF_NULL>                       ; Load ROM patterns 8x14
        TBENTRY 12H, <DF_NULL>                       ; Load ROM patterns 8x8
        TBENTRY 14H, <DF_NULL>                       ; Load ROM charset 8x14
        TBENTRY 22H, <DF_NULL>                       ; Set ROM graph chars 8x14
        TBENTRY 23H, <DF_NULL>                       ; Set ROM graph chars 8x8
        TBENTRY 24H, <DF_NULL>                       ; Set ROM graph chars 8x16
        TBEND

        SPARSE_TABLE     VIO12_FUNCS
        TBENTRY 10H, <DF_NULL>                       ; Get EGA info
        TBENTRY 20H, <DF_NULL>                       ; Alternate PrtSc
        TBENTRY 30H, <DF_NULL>                       ; Select vert. resolution
        TBENTRY 31H, <DF_NULL>                       ; Palette loading
        TBENTRY 32H, <DF_NULL>                       ; Video addressing
        TBENTRY 33H, <DF_NULL>                       ; Gray-scale summing
        TBENTRY 34H, <DF_NULL>                       ; Cursor emulation
        TBENTRY 36H, <DF_NULL>                       ; Video refresh control
        TBENTRY 37H, <DF_NULL>                       ; Mainframe interact. sup
        TBEND

        DENSE_TABLE     MOU_FUNCS, 6DH
        TBENTRY 00H, <DF_NULL>                       ; Reset
        TBENTRY 01H, <DF_NULL>                       ; Show mouse cursor
        TBENTRY 02H, <DF_NULL>                       ; Hide mouse cursor
        TBENTRY 03H, <DF_NULL>                       ; Get position & status
        TBENTRY 04H, <DF_NULL>                       ; Position mouse cursor
        TBENTRY 05H, <DF_NULL>                       ; Get button down data
        TBENTRY 06H, <DF_NULL>                       ; Get button up data
        TBENTRY 07H, <DF_NULL>                       ; Define hor. range
        TBENTRY 08H, <DF_NULL>                       ; Define ver. range
        TBENTRY 09H, <DF_MOU_GCURSOR>                ; Define graphics cursor
        TBENTRY 0AH, <DF_NULL>                       ; Define text cursor
        TBENTRY 0BH, <DF_NULL>                       ; Read motion counters
        TBENTRY 0CH, <DF_INVALID>                    ; Define IRQ routine
        TBENTRY 0DH, <DF_NULL>                       ; Light pen emulation on
        TBENTRY 0EH, <DF_NULL>                       ; Light pen emulation off
        TBENTRY 0FH, <DF_NULL>                       ; Define mickeys/pixel
        TBENTRY 10H, <DF_NULL>                       ; Define update region
        TBENTRY 11H, <DF_NULL>                       ; GENIUS: # of buttons
        TBENTRY 12H, <DF_MOU_LGCURSOR>               ; Graphics cursor block
        TBENTRY 13H, <DF_NULL>                       ; Double-speed threshold
        TBENTRY 14H, <DF_INVALID>                    ; Exchange IRQ routines
        TBENTRY 15H, <DF_NULL>                       ; Get state buffer size
        TBENTRY 16H, <DF_INVALID>                    ; Save state
        TBENTRY 17H, <DF_INVALID>                    ; Restore state
        TBENTRY 18H, <DF_INVALID>                    ; Set event handler
        TBENTRY 19H, <DF_INVALID>                    ; Return event handler
        TBENTRY 1AH, <DF_NULL>                       ; Set sensitivity
        TBENTRY 1BH, <DF_NULL>                       ; Return sensitivity
        TBENTRY 1CH, <DF_NULL>                       ; Set interrupt rate
        TBENTRY 1DH, <DF_NULL>                       ; Define display page
        TBENTRY 1EH, <DF_NULL>                       ; Return display page
        TBENTRY 1FH, <DF_NULL>                       ; Disable mouse driver
        TBENTRY 20H, <DF_NULL>                       ; Enable mouse driver
        TBENTRY 21H, <DF_NULL>                       ; Software reset
        TBENTRY 22H, <DF_NULL>                       ; Set language
        TBENTRY 23H, <DF_NULL>                       ; Get language
        TBENTRY 24H, <DF_NULL>                       ; Get misc. info
        TBENTRY 25H, <DF_NULL>                       ; Get general driver info
        TBENTRY 26H, <DF_NULL>                       ; Get maximum coordinates
        TBENTRY 27H, <DF_NULL>                       ; Get masks & counts
        TBENTRY 28H, <DF_NULL>                       ; Set video mode
        TBENTRY 29H, <DF_INVALID>                    ; Enumerate video modes
        TBENTRY 2AH, <DF_NULL>                       ; Get cursor hot spot
        TBENTRY 2BH, <DF_INVALID>                    ; Load acceleration prof.
        TBENTRY 2CH, <DF_INVALID>                    ; Get acceleration prof.
        TBENTRY 2DH, <DF_INVALID>                    ; Select acc. profile
        TBENTRY 2EH, <DF_INVALID>                    ; Set acc. profile names
        TBENTRY 2FH, <DF_NULL>                       ; Hardware reset
        TBENTRY 30H, <DF_NULL>                       ; Ballpoint information
        TBENTRY 31H, <DF_NULL>                       ; Max. virtual coordinates
        TBENTRY 32H, <DF_NULL>                       ; Get active adv. funcs.
        TBENTRY 33H, <DF_INVALID>                    ; Switch settings etc.
        TBENTRY 34H, <DF_INVALID>                    ; Get initialization file
        TBENTRY 35H, <DF_NULL>                       ; LCD large pointer
        TBENTRY 4DH, <DF_INVALID>                    ; Get copyright string
        TBENTRY 6DH, <DF_INVALID>                    ; Get version string
        TBEND

;
; Buffer for DOS interface
;
BUF_SEG         WORD    ?               ; Current buffer (BUF?_SEG)
BUF_SEL         WORD    ?               ; Current buffer (G_BUF?_SEL)

BUF1_SEG        WORD    ?               ; 1st buffer (64KB)
BUF2_SEG        WORD    ?               ; 2nd buffer (4KB, for swapper)

COUNTRYDATA     LABEL   BYTE
CD_DATE         WORD    ?
CD_CURRENCY     BYTE    5 DUP (?)
CD_1000_SEP     BYTE    2 DUP (?)
CD_DECIMAL_SEP  BYTE    2 DUP (?)
CD_DATE_SEP     BYTE    2 DUP (?)
CD_TIME_SEP     BYTE    2 DUP (?)
CD_FLAGS        BYTE    ?
CD_CURRENCY_N   BYTE    ?
CD_TIME         BYTE    ?
CD_CASEMAP      DWORD   ?
CD_LIST_SEP     BYTE    2 DUP (?)
CD_RESERVED     BYTE    10 DUP (?)

;
; This table contains 256 bits; if bit i is set, byte i is a DBCS
; lead byte.
;
DBCS_LEAD_TAB   WORD    (256 / 8 / 2) DUP (?)

$INTERRUPT      BYTE    "Unexpected interrupt", 0
$NMI            BYTE    "Nonmaskable interrupt (NMI) at ", 0
$BAD_MOU_FUNC   BYTE    "Illegal moucall function code: ", 0
$BAD_DOS_FUNC   BYTE    "Illegal syscall function code: ", 0
$BAD_VIO_FUNC   BYTE    "Illegal viocall function code: ", 0
$DOS_PARAM      BYTE    "Invalid arguments for syscall ", 0
$EMERGENCY      BYTE    "Emergency exit", 0


              IF DEBUG_DOSDUMP
$DOSDUMP        BYTE    "DOSDUMP.000"
$DOSDUMP_C      =       THIS BYTE - 1
                BYTE    0
              ENDIF

SV_DATA         ENDS


SV_CODE         SEGMENT

                .386P

                ASSUME  CS:SV_CODE, DS:NOTHING

                TALIGN  2
CONV_TAB        WORD    D_INVALID       ; DF_INVALID
                WORD    D_NULL          ; DF_NULL
                WORD    D_WRITE         ; DF_WRITE
                WORD    D_PATH          ; DF_PATH
                WORD    D_TEXT          ; DF_TEXT
                WORD    D_READ          ; DF_READ
                WORD    D_SEEK          ; DF_SEEK
                WORD    D_TMP           ; DF_TMP
                WORD    D_INPUT         ; DF_INPUT
                WORD    D_MOVE          ; DF_MOVE
                WORD    D_DUP2          ; DF_DUP2
                WORD    D_CLOSE         ; DF_CLOSE
                WORD    D_INVALID       ; DF_IOCTL (handled before jumping)
                WORD    D_GETCWD        ; DF_GETCWD
                WORD    D_EXIT          ; DF_EXIT
                WORD    D_FFIRST        ; DF_FFIRST
                WORD    D_FNEXT         ; DF_FNEXT
                WORD    D_VERSION       ; DF_VERSION
                WORD    D_EXEC          ; DF_EXEC
                WORD    D_RETCODE       ; DF_RETCODE
                WORD    D_STR_BP        ; DF_STR_BP
                WORD    D_VESA          ; DF_VESA
                WORD    D_VESAINFO      ; DF_VESAINFO
                WORD    D_VESAMODE      ; DF_VESAMODE
                WORD    D_MOU_GCURSOR   ; DF_MOU_GCURSOR
                WORD    D_MOU_LGCURSOR  ; DF_MOU_LGCURSOR
                WORD    D_COUNTRY       ; DF_COUNTRY
                WORD    D_LOCK          ; DF_LOCK
                WORD    D_TRUENAME      ; DF_TRUENAME
                WORD    D_PATH_SI       ; DF_PATH_SI
                WORD    D_DUP           ; DF_DUP
                WORD    D_VIO11         ; DF_VIO11
                WORD    D_VIO12         ; DF_VIO12

                .ERRE   ($-CONV_TAB)/2 EQ DF_LAST+1


;
; All undefined interrupts go here
;
; This cannot happen, as undefined interrupts cannot be called
; from the user program (CPL=3 & DPL=0) and all hardware interrupts
; have their own vector. Yes, the supervisor could issue an
; undefined software interrupt, but it doesn't. At least I hope so.
;

                ASSUME  DS:NOTHING
INTERRUPT:      MOV     AX, G_SV_DATA_SEL
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                LEA     EDX, $INTERRUPT
                CALL    INT_MSG
                JMP     SHORT $

                ASSUME  DS:NOTHING
NMI:            MOV     AX, G_SV_DATA_SEL
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                LEA     EDX, $NMI
                CALL    INT_MSG
                MOV     AX, SS:[ESP+4]          ; CS
                CALL    VWORD
                MOV     AL, ":"
                CALL    VCHAR
                MOV     EAX, SS:[ESP+0]         ; EIP
                CALL    VDWORD
              IF FALSE
                MOV     AL, " "
                CALL    VCHAR
                MOV     EAX, SS:[ESP+8]         ; EFLAGS
                CALL    VDWORD
              ENDIF
                JMP     SHORT $

;
; Display fatal error message (unexpected interrupt / NMI)
;
                ASSUME  DS:SV_DATA
INT_MSG         PROC    NEAR
                MOV     AX, G_VIDEO_SEL
                MOV     ES, AX
                CALL    VCLS
                CALL    VTEXT
                RET
INT_MSG         ENDP


;
; Backward jump here (for processors with BIG prefetch queue)
;
RM_INT1:        XOR     EAX, EAX
                MOV     CR3, EAX                ; Clear TLB
                JMP     FAR PTR RM_INT2         ; Reload CS, real mode!

;
; Call the real-mode interrupt INT_NO
;
                ASSUME  DS:SV_DATA
                TALIGN  4
INT_RM0         PROC    FAR
                CLI
                CMP     PROFIL_COUNT, 0         ; Suspend generating RTC
                JE      SHORT NOPROF            ; interrupts for the profiler
                CALL    PROFIL_SUSPEND
                TALIGN  4
NOPROF:         MOV     PMHWINT_ESP, ESP        ; Save stack pointer
                MOV     PMHWINT_SS, SS          ; and stack segment
                MOV     FRAME_PTR, BP           ; After CLI!!!
                MOV     EAX, CR0
                AND     EAX, CR0_FPU            ; Keep only FPU bits
                MOV     PM_CR0, EAX             ; Save for switching back to PM
                MOV     EAX, CR0
                AND     EAX, NOT CR0_FPU        ; Restore floating point
                OR      EAX, RM_CR0             ; bits of real/virtual mode
                MOV     CR0, EAX
                XOR     AX, AX
                LLDT    AX                      ; Zero LDTR
                CMP     VCPI_FLAG, FALSE        ; Running under VCPI server?
                JNE     INT_VCPI                ; Yes -> let the server do it
                MOV     AX, G_REAL_SEL          ; Load values suitable for
                MOV     DS, AX                  ; real mode into all the
                ASSUME  DS:NOTHING              ; segment registers
                MOV     ES, AX                  ; (see below for CS)
                MOV     FS, AX
                MOV     GS, AX
                MOV     SS, AX
                MOV     EAX, CR0
                AND     EAX, NOT (CR0_PG OR CR0_PE) ; Turn off paging and PM
                MOV     CR0, EAX
                JMP     SHORT RM_INT1           ; Flush prefetch queue
;
; ... real mode ...
;
                ASSUME  DS:NOTHING
                TALIGN  4
RM_INT9::       MOV     AX, G_SV_DATA_SEL
                MOV     DS, AX                  ; Make data accessible
                ASSUME  DS:SV_DATA
                LSS     ESP, PMHWINT_LSS        ; Restore stack
                AND     TSS_BUSY, NOT 2         ; Otherwise exception on LTR
                MOV     AX, G_TSS_SEL
                LTR     AX                      ; Setup task
                CMP     PAGING, FALSE           ; Use paging?
                JE      SHORT RM_INT10          ; No  -> skip
                MOV     EAX, PAGE_DIR_PHYS
                MOV     CR3, EAX                ; Setup page directory
                MOV     EAX, CR0
                OR      EAX, CR0_PG             ; Enable paging
                MOV     CR0, EAX
;
; Common code for `manual' return from real mode and for VCPI
;
RM_INT10:       MOV     EAX, CR0
                AND     EAX, NOT CR0_FPU        ; Restore floating point
                OR      EAX, PM_CR0             ; bits of protected mode
                MOV     CR0, EAX
                RET                             ; FAR return!!!

;
; When running under a VCPI server, we return here from virtual mode
;

                ASSUME  DS:NOTHING
                TALIGN  4
V2P_CONT::      MOV     AX, G_SV_DATA_SEL
                MOV     DS, AX                  ; Make data accessible
                ASSUME  DS:SV_DATA
                MOV     ES, AX
                MOV     FS, AX
                MOV     GS, AX
                LSS     ESP, PMHWINT_LSS        ; Setup stack
                JMP     SHORT RM_INT10          ; Floating point bits of CR0

INT_RM0         ENDP


                ASSUME  DS:SV_DATA
                TALIGN  4
INT_RM          PROC    NEAR
                PUSH    EBP                     ; Save EBP
                PUSHFD                          ; Save flags
                SLDT    AX
                PUSH    AX                      ; Save LDTR
                CALLF16 G_SV_CODE_SEL, INT_RM0  ; FAR call!
                MOV     AX, G_SV_DATA_SEL       ; Initialize all segment
                MOV     DS, AX                  ; registers
                MOV     ES, AX
                MOV     FS, AX
                MOV     GS, AX
;
; Important: Restore LDTR of interrupted program. Real/virtual mode may have
; changed LDTR (INT 15H: extended memory). Actually, we zero LDTR after
; entering real mode. We need the LDTR for popping the segment registers
; of the interrupted program, which may reference the LDT.
;
                POP     AX
                LLDT    AX                      ; Restore LDTR
                CMP     PROFIL_COUNT, 0
                JE      SHORT NOPROF
                CLI                             ; Resume generating RTC
                CALL    PROFIL_RESUME           ; interrupts for the profiler
                TALIGN  4
NOPROF:         POPFD                           ; Restore flags
                POP     EBP                     ; Restore EBP
                RET                             ; Done
INT_RM          ENDP

;
; This is the first interrupt vector used for redirecting IRQ0..IRQ7
; and IRQ8..IRQ15, respectively. These variables are located in SV_CODE
; as they are accessed with unknown segment registers (only CS is known
; to be SV_CODE_SEL).
;
IRQ0_ADD        BYTE    ?                       ; Initialized by INIT_INT
IRQ8_ADD        BYTE    ?                       ; Initialized by INIT_INT


;
; Entry points for hardware interrupts IRQ 0 to IRQ 7.
;

                ASSUME  DS:NOTHING

                IRP     X, <0,1,2,3,4,5,6,7>
                TALIGN  4
PMINT&X:        PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, &X * 0100H + &X     ; Compute real-mode interrupt
                ADD     CL, IRQ0_ADD            ; number (in SV_CODE segment!)
                JMP     PMINT                   ; Common code
                ENDM

;
; Entry points for hardware interrupts IRQ 8 to IRQ 15.
;

                ASSUME  DS:NOTHING

                IRP     X, <8,9,10,11,12,13,14,15>
                TALIGN  4
PMINT&X:        PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, &X * 0100H + &X-8   ; Compute real-mode interrupt
                ADD     CL, IRQ8_ADD            ; number (in SV_CODE segment!)
                JMP     PMINT                   ; Common code
                ENDM

;
; Entry point for software interrupt 10H (video interface).
;
                TALIGN  4
PMINT_10H:      PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, 1010H               ; Real-mode interrupt number
                JMP     SHORT PMINT             ; Common code

;
; Entry point for software interrupt 11H (equipment).
;
                TALIGN  4
PMINT_11H:      PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, 1111H               ; Real-mode interrupt number
                JMP     SHORT PMINT             ; Common code

;
; Entry point for software interrupt 14H (RS-232 interface).
;
                TALIGN  4
PMINT_14H:      PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, 1414H               ; Real-mode interrupt number
                JMP     SHORT PMINT             ; Common code

;
; Entry point for software interrupt 16H (keyboard interface).
;
                TALIGN  4
PMINT_16H:      PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, 1616H               ; Real-mode interrupt number
                JMP     SHORT PMINT             ; Common code

;
; Entry point for software interrupt 17H (printer interface).
;
                TALIGN  4
PMINT_17H:      PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, 1717H               ; Real-mode interrupt number
                JMP     SHORT PMINT             ; Common code

;
; Entry point for software interrupt 21H (DOS interface).
;
                TALIGN  4
PMINT_21H:      PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, 2121H               ; Real-mode interrupt number
                JMP     SHORT PMINT             ; Common code

;
; Entry point for software interrupt 31H (DOS extender interface).
;
                TALIGN  4
PMINT_31H:      PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, 3131H               ; Real-mode interrupt number
                JMP     SHORT PMINT             ; Common code

;
; Entry point for software interrupt 33H (mouse interface).
;
                TALIGN  4
PMINT_33H:      PUSH    EAX                     ; Mock error code
                PUSHAD                          ; Save all general registers
                MOV     CX, 3333H               ; Real-mode interrupt number
                JMP     SHORT PMINT             ; Common code

;
; Common code for protected-mode interrupts (hardware and software interrupts).
;
; Note: this code runs with interrupts masked.
;
                TALIGN  4
PMINT:          PUSH    DS                      ; Save segment registers
                PUSH    ES
                PUSH    FS
                PUSH    GS
              IF SER_FLAG
                SERIAL  "I"
              ENDIF
                CLD
                MOV     AX, G_SV_DATA_SEL
                MOV     DS, AX                  ; Make data segment available
                ASSUME  DS:SV_DATA
                MOV     EBP, ESP                ; Setup stack frame pointer
                SUB     ESP, FRAME_SIZE         ; Local variables
PMINT_STACK_SNAP_SHOT:                          ; For documentation only
                MOV     INT_NO, CL              ; Save interrupt number
                MOV     HW_INT_VEC, CH
                CMP     CH, 8                   ; RTC interrupt?
                JE      RTC_INT                 ; Yes -> profile or reflect
              IF FLOATING_POINT
                CMP     CH, 13                  ; Coprocessor interrupt?
                JE      COPROC                  ; Yes -> map to SIGFPE
SKIP_COPROC:
              ENDIF
                CMP     CL, 21H                 ; DOS call interrupt?
                JE      DOSCALL                 ; Yes ->
                CMP     CL, 10H                 ; Video API interrupt?
                JE      VIOCALL                 ; Yes ->
                CMP     CL, 11H                 ; Equiment interrupt?
                JE      GENERIC_CALL            ; Yes ->
                CMP     CL, 14H                 ; RS-232 API interrupt?
                JE      GENERIC_CALL            ; Yes ->
                CMP     CL, 16H                 ; Keyboard API interrupt?
                JE      GENERIC_CALL            ; Yes ->
                CMP     CL, 17H                 ; Printer API interrupt?
                JE      GENERIC_CALL            ; Yes ->
                CMP     CL, 31H                 ; DOS extender API interrupt?
                JE      EXT_CALL                ; Yes ->
                CMP     CL, 33H                 ; Mouse API interrupt?
                JE      MOUCALL                 ; Yes ->
;
; This is the simple case: just reissue the interrupt in real mode.
;
REFLECT:        CALL    INT_RM
;
; The STI in the following line has been removed in emx 0.8b to fix a stack
; overflow problem with high speed communications and an interrupt controller
; reprogrammed to AEOI mode.
;
;***            STI                             ; At the risk of stack overflow
;
; Back in protected mode, return to interrupted code (and check for signals)
;
PMINT_RET:      CMP     EMERGENCY_FLAG, FALSE   ; Emergency exit pending?
                JE      SHORT SKIP_EMGCY
                CALL    EMERGENCY_EXIT
SKIP_EMGCY:     ADD     ESP, FRAME_SIZE         ; Remove local variables
                CALL    SIG_DELIVER_PENDING
              IF SER_FLAG
                SERIAL  "i"
              ENDIF
                POP     GS                      ; Restore segment registers
                POP     FS
                POP     ES
                POP     DS
                POPAD                           ; Restore general registers
                NOP                             ; Avoid 386 bug
                ADD     ESP, 4                  ; Remove error code
                IRETD                           ; End of interrupt routine
                                                ; 32-bit interrupt gate!!!


;
; Syscall functions
;
                TALIGN  4
SYSCALL_1:      CALL    DO_SYSCALL
                JMP     PMINT_RET

;
; Emergency exit
;
EMERGENCY_EXIT  PROC    NEAR
                MOV     EMERGENCY_FLAG, FALSE
                LEA     EDX, $EMERGENCY
                CALL    OTEXT
                CALL    OCRLF
                MOV     AX, 4CFFH
                INT     21H
EMERGENCY_EXIT  ENDP

;
; Real-time clock interrupt (IRQ8).
;
                TALIGN  4
RTC_INT:        CMP     PROFIL_COUNT, 0
                JE      REFLECT
                CALL    PROFIL_TICK
                JMP     PMINT_RET

;
; Coprocessor interrupt (IRQ13).  Reset the BUSY pin and raise SIGFPE.
;
                TALIGN  4
COPROC:         CMP     MACHINE, MACH_PC        ; PC architecture?
                JNE     SKIP_COPROC             ; No  -> give up
;
; Reset the interrupt condition
;
                MOV     AL, 00H                 ; Send a byte of zeros
                OUT     0F0H, AL                ; to reset the BUSY pin
                MOV     AL, 20H                 ; Send EOI command to the
                OUT     0A0H, AL                ; slave interrupt controller,
                OUT     20H, AL                 ; master interrupt controller
                FNCLEX                          ; Clear exceptions
;
; Raise SIGFPE
;
                MOV     AL, 16                  ; Exception 16: Coproc error
                JMP     EXCEPT_FAKE             ; Fake an exception

COPROC_DONE:    JMP     PMINT_RET

;
; DOS extender interface.
;
                TALIGN  4
EXT_CALL:       STI
                CALL    EXTAPI
                JMP     PMINT_RET

;
; DOS interface.
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
                TALIGN  4
DOSCALL:        STI                             ; Be venturous!
                AND     I_EFLAGS, NOT FLAG_C    ; Clear carry flag (no error)
                MOV     EAX, I_EAX              ; Get EAX (we need only AX)
                CMP     AH, 7FH                 ; Syscall functions?
                JE      SHORT SYSCALL_1         ; Yes ->
                CALL    REGS_TO_RM              ; Set real-mode registers
;
; Apply umask for __creat()
;
                CMP     BYTE PTR RM_AX[1], 3CH  ; Create handle?
                JNE     SHORT SKIP_CREAT        ; No  -> skip
                MOV     BX, PROCESS_PTR         ; Current process
                CMP     BX, NO_PROCESS          ; In user code?
                JE      SHORT SKIP_CREAT        ; No  -> keep mode
                TEST    (PROCESS PTR [BX]).P_UMASK, S_IWRITE
                JZ      SHORT SKIP_CREAT        ; Write perm. -> keep mode
                OR      RM_CX, 1                ; Set read-only attribute
                TALIGN  4
;
; Get conversion flag for INT 21H functions, convert handles if appropriate.
;
SKIP_CREAT:     MOV     INP_HANDLE, NO_FILE_HANDLE  ; Handle not converted
                MOV     BL, BYTE PTR RM_AX[1]   ; AH
                TBGET   DOS_FUNCS               ; Get conversion flag
                CMP     AL, DF_IOCTL            ; Use second table?
                JNE     SHORT DOSCALL_1
                MOV     BL, BYTE PTR RM_AX[0]   ; AL
                TBGET   IOCTL_FUNCS             ; Get conversion flag
DOSCALL_1:      MOV     CONV_FLAG, AX           ; Save conversion flag
                CMP     PROCESS_PTR, NO_PROCESS ; Translate handles?
                JE      SHORT DOSCALL_9         ; No -> skip
                TEST    CONV_FLAG, DFH_IN       ; Convert handle (input)?
                JZ      SHORT DOSCALL_2         ; No -> check for output
                MOV     AX, RM_BX               ; Get process' handle
                MOV     INP_HANDLE, AX          ; Save this value
                CALL    GET_HANDLE              ; Convert to DOS handle
                MOV     RM_BX, AX               ; and store to BX
DOSCALL_2:      TEST    CONV_FLAG, DFH_OUT      ; Convert handle (output)?
                JZ      SHORT DOSCALL_3         ; No -> skip
                MOV     AX, NO_FILE_HANDLE
                CALL    NEW_HANDLE              ; Slot available?
                JNC     SHORT DOSCALL_3         ; Yes -> continue
                MOV     I_EAX, EMFILE           ; Too many open files
                OR      BYTE PTR I_EFLAGS, FLAG_C ; Set carry flag
                JMP     PMINT_RET               ; Return to caller
;
; Check whether identical mapping should be used for handles 0 and 1.
; This can be done after calling GET_HANDLE because all functions
; for which DFM_STDIN or DFM_STDOUT is set, don't use an explicit
; file handle (that's why DFM_STDIN and DFM_STDOUT are required in
; the first place).
;
DOSCALL_3:      TEST    CONV_FLAG, DFM_STDIN    ; Remap handle 0?
                JZ      SHORT DOSCALL_4
                MOV     AX, 0
                CALL    MAP_HANDLE
                JC      SHORT MAP_ERROR
DOSCALL_4:      TEST    CONV_FLAG, DFM_STDOUT   ; Remap handle 1?
                JZ      SHORT DOSCALL_9
                MOV     AX, 1
                CALL    MAP_HANDLE
                JNC     SHORT DOSCALL_9
MAP_ERROR:      MOV     WORD PTR I_EAX+0, AX
                MOV     WORD PTR I_EAX+2, 0
                OR      BYTE PTR I_EFLAGS, FLAG_C ; Set carry flag
                JMP     PMINT_RET

;
; Dispatcher for INT 21H
;
                TALIGN  4
DOSCALL_9:      MOVZX   BX, BYTE PTR CONV_FLAG  ; Get conversion flag
                SHL     BX, 1
                JMP     CONV_TAB[BX]

;
; Dispatcher for other interrupts (use AX as conversion flag)
;
                TALIGN  4
INTCALL:        MOV     CONV_FLAG, AX           ; No conversion at DOSCALL_RET
                MOV     BX, AX
                SHL     BX, 1
                JMP     CONV_TAB[BX]

;
; INT 10H -- video BIOS
;
VIOCALL:        MOV     INP_HANDLE, NO_FILE_HANDLE
                CALL    REGS_TO_RM
                MOV     BL, BYTE PTR RM_AX[1]   ; AH
                TBGET   VIO_FUNCS               ; Get conversion flag
                JMP     SHORT INTCALL

D_VESA:         MOV     BL, BYTE PTR RM_AX[0]   ; AL
                TBGET   VESA_FUNCS              ; Get conversion flag
                JMP     SHORT INTCALL

D_VIO11:        MOV     BL, BYTE PTR RM_AX[0]   ; AL
                TBGET   VIO11_FUNCS             ; Get conversion flag
                JMP     SHORT INTCALL

D_VIO12:        MOV     BL, BYTE PTR RM_BX[0]   ; BL
                TBGET   VIO12_FUNCS             ; Get conversion flag
                JMP     SHORT INTCALL

;
; INT 33H -- mouse driver
;
MOUCALL:        MOV     INP_HANDLE, NO_FILE_HANDLE
                CALL    REGS_TO_RM
                MOV     BX, RM_AX               ; Get AX
                CMP     BH, 0                   ; AH=0?
                JNE     D_INVALID               ; No -> error
                TBGET   MOU_FUNCS               ; Get conversion flag
                JMP     INTCALL

;
; Generic software interrupt, no conversion
;
GENERIC_CALL:   MOV     INP_HANDLE, NO_FILE_HANDLE
                CALL    REGS_TO_RM
                MOV     AX, DF_NULL             ; No conversion
                JMP     INTCALL

;
; Duplicate file handle
;
D_DUP:          MOV     BX, PROCESS_PTR
                CMP     BX, NO_PROCESS          ; Called from user process?
                JE      D_NULL                  ; No -> directly call DOS
                MOV     AX, RM_BX               ; Handle
                CALL    DO_DUP
D_DUP_COMMON:   MOV     WORD PTR I_EAX + 0, AX
                MOV     WORD PTR I_EAX + 2, 0
                JNC     SHORT D_DUP_FIN
                OR      BYTE PTR I_EFLAGS, FLAG_C ; Set carry flag
D_DUP_FIN:      JMP     PMINT_RET               ; DFE not set for D_DUP!

;
; Force duplicate file handle
;
D_DUP2:         MOV     BX, PROCESS_PTR
                CMP     BX, NO_PROCESS          ; Called from user process?
                JE      D_NULL                  ; No -> directly call DOS
                MOV     AX, RM_BX               ; Handle1
                MOV     CX, RM_CX               ; Handle2
                CALL    DO_DUP2
                JMP     D_DUP_COMMON

;
; Invalid function
;
D_INVALID:      MOV     BL, BYTE PTR RM_AX[0]
                LEA     EDX, $BAD_MOU_FUNC      ; MOU function not supported
                CMP     INT_NO, 33H
                JE      SHORT DOSF_ERROR
                MOV     BL, BYTE PTR RM_AX[1]
                LEA     EDX, $BAD_VIO_FUNC      ; VIO function not supported
                CMP     INT_NO, 10H
                JE      SHORT DOSF_ERROR
                LEA     EDX, $BAD_DOS_FUNC      ; DOS function not supported
DOSF_ERROR:     PUSH    BX                      ; Save function number (BL)
                CALL    OTEXT                   ; Display message
                POP     AX                      ; Restore function number (AL)
                CALL    OBYTE                   ; Display hex byte
                MOV     AX, 4CFFH               ; Terminate program (255)
                INT     21H                     ; This is recursive!

;
; Invalid arguments
;
DOSF_PARAM:     LEA     EDX, $DOS_PARAM         ; Invalid arguments
                JMP     DOSF_ERROR              ; Abort

;
; Convert output:
;
; Insert emx version
;
D_VERSION:      CALL    INT_RM                  ; Call DOS
                CALL    REGS_FROM_RM            ; Copy all general registers
                MOV     WORD PTR I_EAX+2, 6D65H ; Indicate emx: "em"
                JMP     DOSCALL_RET             ; Done


;
; Convert input:
;
; DS:EDX points to "$" terminated string
;
                TALIGN  4
D_TEXT:         MOV     AL, "$"                 ; Look for "$"
                MOV     AH, FALSE               ; Not a path name
                XOR     EBX, EBX                ; Don't add
                CALL    G_DELIM
                JMP     DOSCALL_RET             ; Done


;
; Convert input and output:
;
; DS:EDX points to 0 terminated string (path)
; DS:ESI points to DTA (43 bytes)
;
                TALIGN  4
D_FFIRST:       CALL    SET_DTA_1               ; Uses OFFSET_1
                MOV     ESI, I_EDX
                MOV     AL, 00H                 ; Terminator
                MOV     AH, NOT FALSE           ; It's a path name
                XOR     EBX, EBX
                MOV     EDI, OFFSET_2
                MOV     RM_DX, DI               ; Offset of buffer
                CALL    I_DELIM
D_FFIRST_1:     PUSH    I_EDX                   ; Save EDX (argument)
                CALL    INT_RM                  ; Call DOS
                CALL    REGS_FROM_RM            ; Retrieve real-mode registers
                POP     I_EDX                   ; Restore EDX (of caller)
                MOV     AX, I_DS                ; AX:EDI = destination
                MOV     EDI, I_ESI
                MOV     ECX, 43
                CALL    MOVE_FROM_RM            ; Copy from real-mode buffer
                JMP     DOSCALL_RET

;
; Convert input and output:
;
; DS:ESI points to DTA (43 bytes)
;
                TALIGN  4
D_FNEXT:        CALL    SET_DTA_1               ; Uses OFFSET_1
                MOV     AX, I_DS
                MOV     ESI, I_ESI
                MOV     EDI, OFFSET_1
                MOV     ECX, 43
                CALL    MOVE_TO_RM
                JMP     SHORT D_FFIRST_1

;
;
; Convert input:
;
; DS:EDX points to 0 terminated string (path) -> DS:DX
; DS:EDI points to 0 terminated string (path) -> ES:DI
;
                TALIGN  4
D_MOVE:         MOV     ESI, I_EDI
                MOV     AL, 00H                 ; Terminator
                MOV     AH, NOT FALSE           ; It's a path name
                XOR     EBX, EBX
                MOV     EDI, OFFSET_2
                MOV     RM_DI, DI               ; Offset of buffer
                CALL    I_DELIM
                ; fall through to D_PATH
;
; Convert input:
;
; DS:EDX points to 0 terminated string (path)
;
                TALIGN  4
D_PATH:         MOV     AL, 0                   ; Look for zeros
                MOV     AH, NOT FALSE           ; It's a path name
                XOR     EBX, EBX                ; Don't add
                CALL    G_DELIM
                JMP     DOSCALL_RET

;
; Convert input:
;
; DS:ESI points to 0 terminated string (path)
;
D_PATH_SI:      MOV     AL, 0                   ; Look for zeros
                MOV     AH, NOT FALSE           ; It's a path name
                XOR     EBX, EBX                ; Don't add
                MOV     ESI, I_ESI
                MOV     EDI, OFFSET_1
                MOV     RM_SI, DI               ; Offset of buffer
                CALL    I_DELIM
                PUSH    I_ESI                   ; Save ESI (argument)
                CALL    INT_RM                  ; Call DOS
                CALL    REGS_FROM_RM            ; Retrieve real-mode registers
                POP     I_ESI                   ; Restore ESI (of caller)
                JMP     DOSCALL_RET

;
; Convert input/output:
;
; DS:EDX points to 0 terminated string (path) + 13 bytes
;
                TALIGN  4
D_TMP:          MOV     AL, 0                   ; Look for zeros
                MOV     AH, NOT FALSE           ; It's a path name
                MOV     EBX, 13                 ; Add 13 bytes
                CALL    G_DELIM
                MOV     AX, I_DS                ; AX:EDI = destination
                MOV     EDI, I_EDX
                CALL    MOVE_FROM_RM            ; Copy from real-mode buffer
                JMP     DOSCALL_RET

;
; Convert input/output:
;
; DS:EDX points to buffer, length: first byte + 2
;
                TALIGN  4
D_INPUT:        MOV     AX, I_DS
                MOV     ESI, I_EDX
                MOV     ES, AX
                MOVZX   ECX, BYTE PTR ES:[ESI]
                ADD     ECX, 2
                PUSH    ECX
                MOV     EDI, OFFSET_1
                MOV     RM_DX, DI               ; Offset of buffer
                CALL    MOVE_TO_RM
                CALL    INT_RM
                POP     ECX
                MOV     AX, I_DS
                MOV     EDI, I_EDX
                CALL    MOVE_FROM_RM
                JMP     DOSCALL_RET

;
; Convert output:
;
; DS:ESI points to zero terminated string
;
                TALIGN  4
D_GETCWD:       MOV     RM_SI, OFFSET_1         ; Offset of buffer
                CALL    INT_RM
                TEST    RM_FLAGS, FLAG_C
                JNZ     DOSCALL_RET
                MOV     EDI, I_ESI
                MOV     AX, I_DS
                CALL    MOVEZ_FROM_RM
                JMP     DOSCALL_RET

;
; Input (registers of caller):
;   BX     = handle
;   DS:EDX = buffer
;   ECX    = number of bytes
;
; Output (registers of caller):
;   EAX    = number of bytes or error code
;   CY=1   : error
;
; Truncate the file at the current position of the file pointer (in contrast
; to the statement in system.doc). This is required for the implementation
; of _ftruncate().
;
                TALIGN  4
D_WRITE:        MOV     AX, WORD PTR I_EAX
                MOV     N_AX, AX
                MOV     EAX, I_ECX
                MOV     NREQ, EAX
                PUSH    EAX                     ; Save caller's ECX
                MOV     EAX, I_EDX
                MOV     NPTR, EAX
                PUSH    EAX                     ; Save caller's EDX
                MOV     I_EAX, 0                ; Result := 0 (EAX)
D_WRITE1:       MOV     EAX, NREQ
                CMP     EAX, 0FFFFH - OFFSET_1
                JBE     SHORT D_WRITE2
                MOV     AX, 0FE00H              ; 64K - 512
D_WRITE2:       MOV     RM_CX, AX
                MOV     RM_DX, OFFSET_1         ; Offset of buffer
                MOV     AX, I_DS                ; AX:ESI = source
                MOV     ESI, NPTR
                MOVZX   ECX, RM_CX              ; Size of buffer
                MOV     EDI, OFFSET_1
                MOV     RM_DX, DI               ; Offset of buffer
                CALL    MOVE_TO_RM              ; Copy to real-mode buffer
                MOV     AX, N_AX
                MOV     RM_AX, AX
                CALL    INT_RM                  ; Call DOS
                TEST    RM_FLAGS, FLAG_C        ; Error?
                JNZ     SHORT D_WRITE_ERR
                MOV     AX, RM_AX               ; Number of bytes written
                TEST    AX, AX                  ; Zero?
                JZ      SHORT D_WRITE_END       ; Yes -> done
                MOVZX   ECX, AX
                ADD     I_EAX, ECX              ; Update return value
                CMP     AX, RM_CX               ; Less than requested?
                JB      SHORT D_WRITE_END       ; Yes -> done
                ADD     NPTR, ECX
                SUB     NREQ, ECX
                JNZ     SHORT D_WRITE1
                JMP     SHORT D_WRITE_END

D_WRITE_ERR:    MOVZX   EAX, RM_AX              ; Copy error code
                MOV     I_EAX, EAX
D_WRITE_END:    MOV     AL, RM_FLAGS            ; Copy flags
                MOV     BYTE PTR I_EFLAGS[0], AL
                POP     I_EDX                   ; Restore caller's EDX
                POP     I_ECX                   ; Restore caller's ECX
                JMP     DOSCALL_RET             ; Done

;
; Input (registers of caller):
;   BX     = handle
;   DS:EDX = buffer
;   ECX    = number of bytes
;
; Output (registers of caller):
;   EAX    = number of bytes or error code
;   CY=1   : error
;
                TALIGN  4
D_READ:         CMP     BYTE PTR RM_AX[1], 3FH  ; AH=3FH (read handle)?
                JNE     SHORT D_READ_10
                CMP     RM_BX, 0                ; Handle = stdin?
                JNE     SHORT D_READ_10
                TEST    STDIN_TERMIO.C_LFLAG, IDEFAULT
                JNZ     SHORT D_READ_10
                CMP     PROCESS_PTR, NO_PROCESS
                JE      SHORT D_READ_10
                TEST    I_CS, 3                 ; Called from user code?
                JZ      SHORT D_READ_10         ; No  ->
                PUSH    RM_BX
                MOV     RM_AX, 4400H            ; IOCTL: Get device data
                CALL    INT_RM
                TEST    RM_FLAGS, FLAG_C        ; Error?
                JNZ     SHORT D_READ_09         ; Yes -> continue
                TEST    RM_DX, 80H              ; Device?
                JZ      SHORT D_READ_09         ; No  -> continue
                TEST    RM_DX, 01H              ; Console input device?
                JZ      SHORT D_READ_09         ; No  -> continue
                POP     RM_BX                   ; Cleanup stack
                MOV     ES, I_DS
                MOV     EDI, I_EDX
                MOV     ECX, I_ECX
                CALL    TERMIO_READ
                MOV     I_EAX, EAX
                JNC     D_READ_01
                OR      I_EFLAGS, FLAG_C
D_READ_01:      MOV     AX, INP_HANDLE
                CMP     AX, NO_FILE_HANDLE      ; Input handle converted?
                JE      SHORT D_READ_02         ; No -> skip
                MOV     WORD PTR I_EBX, AX      ; Store to (E)BX
D_READ_02:      JMP     PMINT_RET

D_READ_09:      CALL    REGS_TO_RM
                POP     RM_BX
D_READ_10:      MOV     AX, WORD PTR I_EAX
                MOV     N_AX, AX
                MOV     EAX, I_ECX
                MOV     NREQ, EAX
                PUSH    EAX                     ; Save caller's ECX
                MOV     EAX, I_EDX
                MOV     NPTR, EAX
                PUSH    EAX                     ; Save caller's EDX
                MOV     I_EAX, 0                ; 0 bytes read
D_READ1:        MOV     EAX, NREQ
                CMP     EAX, 0FFFFH - OFFSET_1
                JBE     SHORT D_READ2
                MOV     AX, 0FE00H              ; 64K - 512
D_READ2:        MOV     RM_CX, AX
                MOV     RM_DX, OFFSET_1         ; Offset of buffer
                MOV     AX, N_AX
                MOV     RM_AX, AX
                CALL    INT_RM                  ; Call DOS
                TEST    RM_FLAGS, FLAG_C        ; Error?
                JNZ     SHORT D_READ_ERR
                MOVZX   ECX, RM_AX              ; Get number of bytes read
                JECXZ   D_READ3                 ; EOF -> done
                MOV     AX, I_DS                ; AX:EDI = destination
                MOV     EDI, NPTR
                CALL    MOVE_FROM_RM            ; Copy from real-mode buffer
                ADD     I_EAX, ECX              ; Adjust return value (EAX)
                CMP     CX, RM_CX               ; All bytes of this chunk read?
                JNE     SHORT D_READ3           ; No -> done
                ADD     NPTR, ECX               ; Move pointer
                SUB     NREQ, ECX               ; Decrement byte count
                JNZ     SHORT D_READ1           ; Not done -> repeat
D_READ3:        JMP     SHORT D_READ_END

D_READ_ERR:     MOVZX   EAX, RM_AX              ; Copy error code
                MOV     I_EAX, EAX
D_READ_END:     MOV     AL, RM_FLAGS            ; Copy flags
                MOV     BYTE PTR I_EFLAGS[0], AL
                POP     I_EDX                   ; Restore caller's EDX
                POP     I_ECX                   ; Restore caller's ECX
                JMP     DOSCALL_RET             ; Done


;
; Convert input/output:
;
; EDX -> CX:DX
; Call DOS
; DX:AX -> EAX
;
                TALIGN  4
D_SEEK:         MOV     EAX, I_EDX              ; Get EDX
                MOV     RM_DX, AX               ; Store lower word
                SHR     EAX, 16                 ; Get upper word
                MOV     RM_CX, AX               ; Store upper word
                CALL    INT_RM                  ; Call DOS
                XOR     EAX, EAX                ; Clear upper word of EAX
                TEST    RM_FLAGS, FLAG_C        ; Error?
                JNZ     SHORT D_SEEK_1          ; Yes -> no conversion
                MOV     AX, RM_DX               ; Get DX
                SHL     EAX, 16                 ; and put it into upper word
D_SEEK_1:       MOV     AX, RM_AX               ; Put AX into lower word
                MOV     I_EAX, EAX              ; of EAX
                MOV     AL, RM_FLAGS            ; Copy flags
                MOV     BYTE PTR I_EFLAGS[0], AL
                JMP     DOSCALL_RET             ; Done

;
; Close a file handle
;
; Note: When called from user code no translation of error codes
;       is done because DO_CLOSE already returns errno value.
;       DO_CLOSE in turn may invoke this code via INT 21H.  In that
;       case (called from the kernel), error translation is performed,
;       but the handle is not translated.
;
                TALIGN  4
D_CLOSE:        MOV     BX, PROCESS_PTR
                CMP     BX, NO_PROCESS          ; Called from user process?
                JE      D_NULL                  ; No -> no translation
                MOV     AX, RM_BX               ; This is the handle
                CALL    DO_CLOSE                ; Close the handle
                JNC     SHORT D_CLOSE_FIN       ; No error -> done
D_CLOSE_ERROR:  MOV     WORD PTR I_EAX+0, AX    ; Return errno in EAX
                MOV     WORD PTR I_EAX+2, 0
                OR      BYTE PTR I_EFLAGS, FLAG_C ; Set carry flag
D_CLOSE_FIN:    JMP     PMINT_RET               ; Don't translate error code!

;
; Exit
;
                TALIGN  4
D_EXIT:         TEST    I_CS, 3                 ; Called from user program?
                JZ      SHORT D_EXIT_QUIT       ; No  -> exit to DOS
                MOV     SI, PROCESS_PTR         ; Current process
                CMP     SI, NO_PROCESS          ; No current process?
                JE      SHORT D_EXIT_QUIT       ; Yes -> impossible (go DOS)
                ASSUME  SI:PTR PROCESS
;
; ...to do: process 0 becomes parent process of all children
;
                MOV     AL, BYTE PTR I_EAX      ; Get return code
                MOV     [SI].P_RC, AL           ; Store it
                MOV     EAX, [SI].P_PPID        ; Get parent process ID
                TEST    EAX, EAX                ; Root process?
                JZ      SHORT D_EXIT_QUIT       ; Yes -> return to DOS
                CALL    FIND_PROCESS            ; Find parent process
                CMP     BX, NO_PROCESS          ; Does parent process exist?
                JZ      SHORT D_EXIT_QUIT       ; No  -> cannot happen, quit
                ASSUME  BX:PTR PROCESS
                MOV     PROCESS_PTR, BX         ; Switch to parent process
                MOV     PROCESS_SIG, BX
                MOV     EAX, I_EAX              ; Get return code
                AND     EAX, 0FFH               ; Use only lower 8 bits
                TEST    [BX].P_FLAGS, PF_PSEUDO_ASYNC   ; Pseudo async?
                JNZ     SHORT D_EXIT_PSEUDO     ; Yes -> don't overwrite PID
                MOV     [BX].P_SPAWN_RC, EAX    ; Store the return code
                JMP     SHORT D_EXIT_RESTORE    ; Continue
D_EXIT_PSEUDO:  MOV     [BX].P_RC, AL           ; Store the return code
D_EXIT_RESTORE: CALL    REST_PROCESS            ; Restore parent process
                MOV     BX, SI                  ; The dying process
                PUSH    [BX].P_NUMBER           ; Save for FPUEMU_ENDPROC
                TEST    [BX].P_FLAGS, PF_DEBUG  ; In debugging mode?
                JZ      SHORT D_EXIT_1          ; No -> kill process
                CALL    ZOMBIE_PROCESS          ; Make process defunct
                JMP     SHORT D_EXIT_2
D_EXIT_1:       CALL    REMOVE_PROCESS          ; Thoroughly kill process
D_EXIT_2:       CALL    BREAK_AFTER_IRET        ; Allow debugging
                POP     EAX                     ; P_NUMBER of the dead process
                CALL    FPUEMU_ENDPROC          ; Notify FPU emulator
                JMP     DOSCALL_RET             ; Continue with parent
                ASSUME  BX:NOTHING
                ASSUME  SI:NOTHING
;
; Exit to DOS
;
D_EXIT_QUIT:    CMP     FP_FLAG, FP_387         ; 387 available?
                JNE     SHORT EQUIT1            ; No  -> skip
                FNINIT                          ; Initialize
EQUIT1:         CALL    DEBUG_QUIT              ; Swapper statistics
                XOR     EAX, EAX
                MOV     DR7, EAX                ; Avoid unexpected behaviour
                JMP     SHORT D_NULL

;
; No conversion
;
                TALIGN  4
D_NULL:         CALL    INT_RM                  ; Call DOS
                CALL    REGS_FROM_RM            ; Copy all general registers
                JMP     DOSCALL_RET             ; Done

;
; Just check for supervisor code
;
                TALIGN  4
D_RETCODE:      TEST    I_CS, 3                 ; Called from supervisor?
                JNZ     D_INVALID               ; No  -> bad function code
                JMP     SHORT D_NULL            ; No conversion

;
; Load and execute (a DOS) program
;
                TALIGN  4
D_EXEC:         TEST    I_CS, 3                 ; Called from supervisor?
                JNZ     D_INVALID               ; No  -> bad function code
                MOV     AX, SV_DATA             ; Load real-mode segment
                MOV     RM_DS, AX               ; registers
                MOV     RM_ES, AX
                CALL    INT_RM
                MOV     AX, RM_AX
                MOV     WORD PTR I_EAX, AX
                MOV     AL, RM_FLAGS
                MOV     BYTE PTR I_EFLAGS[0], AL
                JMP     DOSCALL_RET

;
; Convert input:
;
; EBP points to null-terminated string (-> ES:BP)
;
                TALIGN  4
D_STR_BP:       MOV     AL, 0                   ; Look for 0
                MOV     AH, FALSE               ; Not a path name
                XOR     EBX, EBX                ; Don't add
                MOV     ESI, I_EBP
                MOV     EDI, OFFSET_1
                MOV     RM_BP, DI               ; Offset of buffer
                CALL    I_DELIM
                PUSH    I_EBP                   ; Save EBP (of caller)
                CALL    INT_RM                  ; Call interrupt
                CALL    REGS_FROM_RM            ; Retrieve real-mode registers
                POP     I_EBP                   ; Restore EBP (of caller)
                JMP     DOSCALL_RET             ; Done


;
; Convert output of VESA function 0.
;
; EDI (-> ES:DI) points to a 256-byte buffer on entry which is filled-in
; by the function.  There are two pointers to secondary buffers in the
; buffer.  We have to copy the data pointed to by these pointers into
; the process' address space.
;
                TALIGN  4
D_VESAINFO:     MOV     RM_DI, OFFSET_1         ; Offset of buffer
                PUSH    I_EDI                   ; Save EDI (of caller)
                CALL    INT_RM                  ; Call interrupt
                CALL    REGS_FROM_RM            ; Retrieve real-mode registers
                POP     I_EDI                   ; Restore EDI (of caller)
;
; Copy 256-byte buffer
;
                MOV     AX, I_DS                ; AX:EDI = destination
                MOV     EDI, I_EDI
                MOV     ECX, 256
                CALL    MOVE_FROM_RM            ; Copy from real-mode buffer
                CMP     RM_AX, 004FH            ; Successful?
                JNE     DOSCALL_RET             ; No -> don't convert pointers
;
; Allocate a page in the process address space for the secondary buffers
;
                MOV     DI, PROCESS_PTR
                ASSUME  DI:PTR PROCESS
                MOV     EAX, [DI].P_VESAINFO_PTR
                TEST    EAX, EAX
                JNZ     SHORT DVI_REUSE
                MOV     ECX, 1
                CALL    ADD_PAGES
                TEST    EAX, EAX
                JZ      SHORT DVI_FAIL
                MOV     [DI].P_VESAINFO_PTR, EAX
DVI_REUSE:      MOV     EDI, EAX
;
; Copy the OEM name
;
                MOV     ES, I_DS
                MOV     ESI, I_EDI
                XCHG    EAX, ES:[ESI+06H]       ; Pointer to OEM name
                CALL    ACCESS_LOWMEM
                MOV     AX, G_LOWMEM_SEL
                MOV     FS, AX
                MOV     ECX, 128                ; Maximum string length
DVI_OEM_1:      MOV     AL, FS:[EBX]
                TEST    AL, AL
                JZ      SHORT DVI_OEM_2
                INC     EBX
                STOS    BYTE PTR ES:[EDI]
                LOOP    DVI_OEM_1
DVI_OEM_2:      XOR     AL, AL
                STOS    BYTE PTR ES:[EDI]
;
; Copy the mode list
;
                INC     EDI                     ; Make the address even
                AND     EDI, NOT 1
                MOV     EAX, EDI
                XCHG    EAX, ES:[ESI+0EH]       ; Pointer to video modes
                CALL    ACCESS_LOWMEM
                MOV     ECX, (4096 - 128) / 2 - 1
DVI_MODES_1:    MOV     AX, FS:[EBX]
                CMP     AX, 0FFFFH
                JE      SHORT DVI_MODES_2
                ADD     EBX, 2
                STOS    WORD PTR ES:[EDI]
                LOOP    DVI_MODES_1
DVI_MODES_2:    MOV     AX, 0FFFFH
                STOS    WORD PTR ES:[EDI]
                JMP     DOSCALL_RET             ; Done

;
; Function call failed, invalidate the two pointers.
;
DVI_FAIL:       MOV     ES, I_DS
                MOV     ESI, I_EDI
                MOV     DWORD PTR ES:[ESI+06H], 0
                MOV     DWORD PTR ES:[ESI+0EH], 0
                JMP     DOSCALL_RET

                ASSUME  DI:NOTHING

;
; Convert output of VESA function 1.
;
; EDI (-> ES:DI) points to a 256-byte buffer on entry which is filled-in
; by the function.  There's one pointer in the buffer which points to
; a function.  Set that pointer to NULL.
;
                TALIGN  4
D_VESAMODE:     MOV     RM_DI, OFFSET_1         ; Offset of buffer
                PUSH    I_EDI                   ; Save EDI (of caller)
                CALL    INT_RM                  ; Call interrupt
                CALL    REGS_FROM_RM            ; Retrieve real-mode registers
                POP     I_EDI                   ; Restore EDI (of caller)
;
; Copy 256-byte buffer
;
                MOV     AX, I_DS                ; AX:EDI = destination
                MOV     EDI, I_EDI
                MOV     ECX, 256
                CALL    MOVE_FROM_RM            ; Copy from real-mode buffer
                MOV     ES, I_DS
                MOV     ESI, I_EDI
;
; Invalidate pointer to window positioning function (same as AX=4F05H)
;
                MOV     DWORD PTR ES:[ESI+0CH], 0
                JMP     DOSCALL_RET             ; Done

;
; Convert input
;
; EDX points 64-byte buffer (-> ES:DX)
;
                TALIGN  4
D_MOU_GCURSOR:  MOV     AX, I_DS
                MOV     ESI, I_EDX
                PUSH    ESI                     ; Save EDX (of caller)
                MOV     ES, AX                  ; Source: AX:ESI
                MOV     ECX, 64                 ; Copy 2*16 16-bit words
                MOV     EDI, OFFSET_1           ; to real-mode buffer
                MOV     RM_DX, DI               ; Offset of buffer
                CALL    MOVE_TO_RM
                CALL    INT_RM
                POP     I_EDX                   ; Restore EDX (of caller)
                JMP     DOSCALL_RET

;
; Convert input
;
; EDX points to a buffer (-> ES:DX).  The size of the buffer is
; BH * CH * 4 bytes.
;
                TALIGN  4
D_MOU_LGCURSOR: MOV     AL, BYTE PTR RM_BX[1]   ; BH
                MUL     BYTE PTR RM_CX[1]       ; CH
                MOVZX   ECX, AX                 ; ECX = BH * CH
                SHL     ECX, 2                  ; Multiply by 4
                MOV     AX, I_DS
                MOV     ESI, I_EDX
                PUSH    ESI                     ; Save EDX (of caller)
                MOV     ES, AX                  ; Source: AX:ESI
                MOV     EDI, OFFSET_1           ; to real-mode buffer
                MOV     RM_DX, DI               ; Offset of buffer
                CALL    MOVE_TO_RM
                CALL    INT_RM
                POP     I_EDX                   ; Restore EDX (of caller)
                JMP     DOSCALL_RET

;
; Convert output:
;
; DS:EDX (input) points to buffer, length: 34 bytes
;
                TALIGN  4
D_COUNTRY:      CMP     RM_DX, 0FFFFH
                JE      SHORT D_COUNTRY_SET
                MOV     RM_DX, OFFSET_1
                CALL    INT_RM
                MOV     AX, I_DS
                MOV     EDI, I_EDX
                MOV     ECX, 34
                CALL    MOVE_FROM_RM
D_COUNTRY_1:    CALL    REGS_FROM_RM
                JMP     DOSCALL_RET

D_COUNTRY_SET:  CALL    INT_RM
                JMP     D_COUNTRY_1

;
; Convert input:
;
; EDX -> CX:DX
; EDI -> SI:DI
;
                TALIGN  4
D_LOCK:         MOV     EAX, I_EDX
                PUSH    EAX
                MOV     RM_DX, AX
                SHR     EAX, 16
                MOV     RM_CX, AX
                MOV     EAX, I_EDI
                PUSH    EAX
                MOV     RM_DI, AX
                SHR     EAX, 16
                MOV     RM_SI, AX
                CALL    INT_RM
                CALL    REGS_FROM_RM
                POP     I_EDI
                POP     I_EDX
                JMP     DOSCALL_RET

;
; Convert input:  ASCIZ ESI -> DS:SI, EDI -> ES:DI
; Convert output: ASCIZ EDI
; 
                TALIGN  4
D_TRUENAME:     MOV     AL, 0                   ; Look for 0
                MOV     AH, NOT FALSE           ; It's a path name
                XOR     EBX, EBX                ; Don't add
                MOV     ESI, I_ESI
                MOV     EDI, OFFSET_2
                MOV     RM_SI, DI               ; Offset of buffer
                CALL    I_DELIM
                MOV     RM_DI, OFFSET_1
                PUSH    I_ESI                   ; Save EDI and ESI (of caller)
                PUSH    I_EDI
                CALL    INT_RM
                CALL    REGS_FROM_RM
                POP     I_EDI
                POP     I_ESI
                TEST    I_EFLAGS, FLAG_C        ; Error?
                JNZ     SHORT D_TRUENAME_1      ; Yes -> skip
                MOV     AX, I_ES
                MOV     EDI, I_EDI
                CALL    MOVEZ_FROM_RM           ; Copies from OFFSET_1
D_TRUENAME_1:   JMP     DOSCALL_RET


;
; Return from DOS call
;
                TALIGN  4
DOSCALL_RET:    MOV     AX, INP_HANDLE
                CMP     AX, NO_FILE_HANDLE      ; Input handle converted?
                JE      SHORT DOSCALL_RET1      ; No -> skip
                MOV     WORD PTR I_EBX, AX      ; Store to (E)BX
DOSCALL_RET1:   TEST    CONV_FLAG, DFH_OUT      ; New handle?
                JZ      SHORT DOSCALL_RET2
                CMP     PROCESS_PTR, NO_PROCESS ; Called from user process?
                JE      SHORT DOSCALL_RET2      ; No -> skip
                TEST    I_EFLAGS, FLAG_C        ; Error?
                JNZ     SHORT DOSCALL_RET2      ; Yes -> skip
                MOV     AX, WORD PTR I_EAX      ; Get DOS handle
                CALL    NEW_HANDLE              ; New slot
                MOV     I_EAX, EAX              ; Return translated handle
DOSCALL_RET2:   TEST    CONV_FLAG, DFE          ; Convert error code?
                JZ      SHORT DOSCALL_RET3      ; No  -> skip
                TEST    I_EFLAGS, FLAG_C        ; Error?
                JZ      SHORT DOSCALL_RET3      ; No  -> skip
                MOV     AX, WORD PTR I_EAX      ; Get error code
                CALL    DOS_ERROR_TO_ERRNO      ; Translate error code
                MOV     I_EAX, EAX              ; Put errno into EAX
DOSCALL_RET3:   JMP     PMINT_RET

                ASSUME  BP:NOTHING

;
; In:   EBX     Number of bytes to move beyond terminator
;       AL      Terminator
;       AH      Path name flag
;
; Out:  ECX     Buffer size
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
G_DELIM         PROC    NEAR
                MOV     ESI, I_EDX
                MOV     EDI, OFFSET_1
                MOV     RM_DX, DI               ; Offset of buffer
                CALL    I_DELIM
                PUSH    ECX                     ; Save ECX (buffer size)
                PUSH    I_EDX                   ; Save EDX (argument)
                CALL    INT_RM                  ; Call DOS
                CALL    REGS_FROM_RM            ; Retrieve real-mode registers
                POP     I_EDX                   ; Restore EDX (of caller)
                POP     ECX                     ; Restore ECX (buffer size)
                RET
                ASSUME  BP:NOTHING
G_DELIM         ENDP


;
; In:   ESI     Pointer to argument
;       EDI     Offset in buffer
;       EBX     Number of bytes to move beyond terminator
;       AL      Terminator
;       AH      Path name flag (for truncation). Non-zero if path name
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
I_DELIM         PROC    NEAR
                PUSH    AX                      ; Save terminator and flag
                PUSH    EDI                     ; Save offset
                MOV     ES, I_DS                ; ES:EDI points to argument
                MOV     ECX, 0FFFFH
                SUB     ECX, EDI                ; Maximum length
                MOV     EDI, ESI
                CLD                             ; Incrementing
                REPNE   SCAS BYTE PTR ES:[EDI]  ; Look for terminator
                JECXZ   IDELIM_2                ; Not found -> abort
                MOV     ECX, EDI
                SUB     ECX, ESI                ; Length of string
                ADD     ECX, EBX                ; Add
                MOV     AX, I_DS                ; AX:ESI = source
                POP     EDI                     ; Offset
                CALL    MOVE_TO_RM              ; Copy to real-mode buffer
                POP     AX                      ; Terminator and flag
                CMP     AH, FALSE               ; Path name?
                JE      SHORT IDELIM_1          ; No  -> return
                PUSH    ES
                MOV     ES, BUF_SEL
                CALL    TRUNCATE
                POP     ES
IDELIM_1:       RET

IDELIM_2:       POP     EDI
                POP     AX
                JMP     DOSF_PARAM
                ASSUME  BP:NOTHING
I_DELIM         ENDP

;
; Set disk transfer address to OFFSET_1
;
SET_DTA_1       PROC    NEAR
                PUSH    RM_AX                   ; Save real-mode AX
                PUSH    RM_DX                   ; Save real-mode DX
                MOV     RM_AX, 1A00H            ; Set disk transfer address
                MOV     RM_DX, OFFSET_1         ; to OFFSET_1
                CALL    INT_RM                  ; Call DOS
                POP     RM_DX                   ; Restore real-mode DX
                POP     RM_AX                   ; Restore real-mode AX
                RET
SET_DTA_1       ENDP


;
; Copy protected-mode registers to real-mode registers
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
                TALIGN  4
REGS_TO_RM      PROC    NEAR
                IRP     R, <AX,BX,CX,DX,SI,DI,BP>
                MOV     AX, WORD PTR I_E&R      ; Load general real-mode
                MOV     RM_&R, AX               ; registers
                ENDM
                MOV     AX, BUF_SEG             ; Load real-mode segment
                MOV     RM_DS, AX               ; registers
                MOV     RM_ES, AX
                RET
                ASSUME  BP:NOTHING
REGS_TO_RM      ENDP

;
; Copy real-mode registers to protected-mode stack frame
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
                TALIGN  4
REGS_FROM_RM    PROC    NEAR
                IRP     R, <AX,BX,CX,DX,SI,DI,BP>
                MOV     AX, RM_&R
                MOV     WORD PTR I_E&R, AX
                ENDM
                MOV     AL, RM_FLAGS
                MOV     BYTE PTR I_EFLAGS[0], AL
                RET
                ASSUME  BP:NOTHING
REGS_FROM_RM    ENDP



;
; Copy buffer from protected mode to real mode
;
; In:   AX:ESI  Source (protected mode)
;       EDI     OFFSET_1 or OFFSET_2
;       ECX     Number of bytes
;

                ASSUME  DS:SV_DATA

                TALIGN  4
MOVE_TO_RM      PROC    NEAR PUBLIC
                PUSH    DS
                PUSH    ES
                PUSH    EAX
                PUSH    ECX
                PUSH    EDX
                PUSH    ESI
                PUSH    EDI
                MOV     ES, BUF_SEL
                MOV     DS, AX
                ASSUME  DS:NOTHING
                CLD
                MOV     EAX, ECX
                SHR     ECX, 2
                REP     MOVS DWORD PTR ES:[EDI], DWORD PTR DS:[ESI]
                MOV     ECX, EAX
                AND     ECX, 3
                REP     MOVS BYTE PTR ES:[EDI], BYTE PTR DS:[ESI]
                POP     EDI
                POP     ESI
                POP     EDX
                POP     ECX
                POP     EAX
                POP     ES
                POP     DS
                RET
MOVE_TO_RM      ENDP


;
; Copy buffer from real mode to protected mode
;
; In:   AX:EDI  Destination address
;       ECX     Number of bytes
;
; Note: always uses OFFSET_1
;

                ASSUME  DS:SV_DATA

MOVE_FROM_RM    PROC    NEAR PUBLIC
                PUSH    DS
                PUSH    ES
                PUSH    EAX
                PUSH    ECX
                PUSH    ESI
                PUSH    EDI
                MOV     ES, AX
                MOV     ESI, OFFSET_1
                MOV     DS, BUF_SEL
                ASSUME  DS:NOTHING
                CLD
                MOV     EAX, ECX
                SHR     ECX, 2
                REP     MOVS DWORD PTR ES:[EDI], DWORD PTR DS:[ESI]
                MOV     ECX, EAX
                AND     ECX, 3
                REP     MOVS BYTE PTR ES:[EDI], BYTE PTR DS:[ESI]
                POP     EDI
                POP     ESI
                POP     ECX
                POP     EAX
                POP     ES
                POP     DS
                RET
MOVE_FROM_RM    ENDP


;
; Copy zero-terminated buffer from real mode to protected mode
;
; In:   AX:EDI  Destination address
;
; Note: always uses OFFSET_1
;

                ASSUME  DS:SV_DATA

MOVEZ_FROM_RM   PROC    NEAR
                PUSH    DS
                PUSH    ES
                PUSH    EAX
                PUSH    ESI
                PUSH    EDI
                MOV     ES, AX
                MOV     ESI, OFFSET_1
                MOV     DS, BUF_SEL
                ASSUME  DS:NOTHING
                CLD
                TALIGN  4
MZFRM1:         LODS    BYTE PTR DS:[ESI]
                STOS    BYTE PTR ES:[EDI]
                TEST    AL, AL
                JNZ     SHORT MZFRM1
                POP     EDI
                POP     ESI
                POP     EAX
                POP     ES
                POP     DS
                RET
MOVEZ_FROM_RM   ENDP

;
; Set a breakpoint to the return address of the current interrupt/exception
; frame (only if -S option is used)
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
                TALIGN  4
BREAK_AFTER_IRET PROC    NEAR
                CMP     STEP_FLAG, FALSE
                JE      SHORT BAI_RET
                MOV     DX, I_CS
                MOV     EAX, I_EIP
                CALL    SET_BREAKPOINT
                CALL    INS_BREAKPOINTS
BAI_RET:        RET
                ASSUME  BP:NOTHING
BREAK_AFTER_IRET ENDP


;
; Set the DBCS lead byte table.  This function must be called from PMINT.
;
; Out: AX       Zero if successful, non-zero on error
;
                ASSUME  DS:SV_DATA
GET_DBCS_LEAD   PROC    NEAR
;
; First clear all bits in case we can't retrieve the DBCS lead bytes
;
                PUSH    DS
                POP     ES
                MOV     DI, DBCS_LEAD_TAB
                MOV     CX, 256 / 8 / 2
                MOV     AX, 0
                REP     STOSW
;
; Call DOS to get the DBCS lead bytes
;
                MOV     RM_AX, 6300H
                MOV     RM_DS, 0FFFEH
                MOV     RM_SI, 0FFFFH
                CALL    INT_RM
;
; Note that some DOS versions return AL=0 even if this function is
; not supported.  Do some sanity checking on DS:SI
;
                CMP     BYTE PTR RM_AX[0], 0
                JNE     SHORT GDL_FAIL
                CMP     RM_SI, -12
                JA      SHORT GDL_FAIL
                MOV     AX, RM_DS
                CMP     AX, 0FFFEH
                JE      SHORT GDL_FAIL
                SHL     EAX, 16
                MOV     AX, RM_SI
                CALL    ACCESS_LOWMEM
                MOV     ESI, EBX
                MOV     AX, G_LOWMEM_SEL
                MOV     ES, AX
                MOV     CX, 6
GDL_CHECK1:     MOV     AX, ES:[EBX]
                TEST    AX, AX
                JZ      SHORT GDL_CHECK1_OK
                CMP     AL, 80H
                JB      SHORT GDL_FAIL
                CMP     AL, AH
                JA      SHORT GDL_FAIL
                ADD     EBX, 2
                LOOP    GDL_CHECK1
GDL_FAIL:       MOV     AX, 1
                RET

;
; The table looks OK; now set the bits
;
GDL_CHECK1_OK:  MOV     CX, 0
GDL_LOOP1:      MOV     AX, ES:[ESI]
                TEST    AX, AX
                JZ      SHORT GDL_DONE
                ADD     ESI, 2
                MOV     CL, AL
GDL_LOOP2:      BTS     DBCS_LEAD_TAB, CX
                CMP     CL, AH
                JAE     SHORT GDL_LOOP1
                INC     CL
                JMP     SHORT GDL_LOOP2

GDL_DONE:       XOR     AX, AX
                RET

GET_DBCS_LEAD   ENDP

SV_CODE         ENDS


;
; Yes, this module contains real-mode code.
;
INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
;
;

;
; Use backward jump to this code for processors with BIG prefetch queue
;
                TALIGN  4
INT_BACK_1:     JMPF16  G_SV_CODE_SEL, RM_INT9

;
; Generic software interrupt without conversion
;
                ASSUME  DS:SV_DATA
FLUSH2:         CALL    GET_REGS                ; Load registers from variables
                ASSUME  DS:NOTHING
INT_OP_2        LABEL   BYTE
                INT     0                       ; Call interrupt routine
                CALL    PUT_REGS                ; Store registers to variables
                JMP     SHORT INT_BACK          ; Back to protected mode

;
; Hardware interrupt
;
                ASSUME  DS:NOTHING
                TALIGN  4
FLUSH1:         PUSH    ES
                PUSH    BP
INT_OP_1        LABEL   BYTE
                INT     0
                STI                             ; Allow interrupts here
                POP     BP
                POP     ES
                CMP     ES:HW_INT_VEC, 01H      ; Keyboard interrupt (IRQ1)?
                JNE     SHORT POSTINT_10
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                TEST    STDIN_TERMIO.C_LFLAG, IDEFAULT
                JNZ     SHORT POSTINT_10
                TEST    STDIN_TERMIO.C_LFLAG, ISIG
                JZ      SHORT POSTINT_10
                CALL    POLL_KEYBOARD
                ASSUME  DS:NOTHING
POSTINT_10:
;
;
;
                ASSUME  DS:NOTHING
INT_BACK:       MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                CALL    CHECK_A20               ; A20 enabled?
                TEST    AX, AX
                JNZ     SHORT INT_BACK_GO       ; Yes -> skip
                CALL    A20_ON                  ; Enable A20
INT_BACK_GO:    CLI
                CMP     VCPI_FLAG, FALSE        ; VCPI?
                JNE     INT_BACK_VCPI           ; Yes -> use server
                LGDT    GDT_PTR
                LIDT    IDT_PTR
                MOV     EAX, CR0
                OR      EAX, CR0_PE             ; Enable protected mode
                MOV     CR0, EAX
                JMP     INT_BACK_1              ; Flush prefetch queue


                TALIGN  4
RM_INT2         LABEL   FAR
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                MOV     FS, AX
                MOV     GS, AX
                MOV     AX, RM_STACK
                MOV     SS, AX
                LEA     SP, RM_TOS
                LIDT    REAL_IDT_PTR
;
; Entrypoint from VCPI server
;
V2V_CONT:       MOV     BP, FRAME_PTR           ; Access registers of prot mode
                MOV     AX, SV_STACK
                MOV     ES, AX
                CMP     MACHINE, MACH_PC        ; PC?
                JE      SHORT INT_BEG_1         ; Yes -> keep A20 enabled
                CMP     MACHINE, MACH_INBOARD   ; Inboard 386/PC?
                JE      SHORT INT_BEG_1         ; Yes -> keep A20 enabled
                CALL    A20_OFF                 ; No  -> disable A20
INT_BEG_1:      MOV     AL, ES:INT_NO           ; Uses BP!
                CMP     AL, 10H
                JE      VIO_INT
                CMP     AL, 11H
                JE      SOFTWARE_INT
                CMP     AL, 14H
                JE      SOFTWARE_INT
                CMP     AL, 16H
                JE      SOFTWARE_INT
                CMP     AL, 17H
                JE      SOFTWARE_INT
                CMP     AL, 21H
                JE      SHORT DOS_INT
                CMP     AL, 33H
                JE      SOFTWARE_INT
                MOV     CS:INT_OP_1+1, AL       ; Self-modifying code,
                JMP     FLUSH1                  ; flush prefetch queue!



                ASSUME  DS:SV_DATA

                TALIGN  4
DOS_INT:        STI                             ; Enable interrupts
                MOV     AX, ES:RM_AX
                CMP     AX, 7F16H               ; Poll keyboard?
                JE      SHORT DOS_INT_PK
                CMP     AX, 7F2AH               ; __nls_memupr()?
                JE      SHORT DOS_INT_MEMUPR
                PUSH    AX
                CMP     AH, 4BH                 ; Exec?
                JNE     SHORT DOS_INT_1
                PUSH    ES
                CALL    CLEANUP_INT             ; Cleanup interrupts
                POP     ES
DOS_INT_1:      CALL    GET_REGS                ; Load registers from variables
                ASSUME  DS:NOTHING
                CMP     AH, 4CH                 ; Terminate process?
                JE      EXIT                    ; Yes ->
                INT     21H
                CALL    PUT_REGS
                POP     AX
                CMP     AH, 4BH                 ; Exec?
                JNE     SHORT DOS_INT_2
                CALL    INIT_INT                ; Reinitialize interrupts
DOS_INT_2:      JMP     INT_BACK


DOS_INT_PK:     CALL    POLL_KEYBOARD
                JMP     SHORT DOS_INT_2

;
; Real-mode part of __nls_memupr()
;
                ASSUME  DS:SV_DATA

DOS_INT_MEMUPR: CMP     ES:RM_CX, 0
                JE      SHORT DOS_INT_2
                LEA     DX, COUNTRYDATA
                MOV     AX, 3800H
                INT     21H
                JC      SHORT DOS_INT_2
                MOV     BX, ES:RM_DX
                MOV     CX, ES:RM_CX
                MOV     ES, BUF_SEG
MEMUPR_LOOP:    MOV     AL, ES:[BX]
                TEST    AL, AL
                JS      SHORT MEMUPR_HIGH
                CMP     AL, "a"
                JB      SHORT MEMUPR_NEXT
                CMP     AL, "z"
                JA      SHORT MEMUPR_NEXT
                SUB     AL, "a" - "A"
MEMUPR_STORE:   MOV     ES:[BX], AL
MEMUPR_NEXT:    INC     BX
                LOOP    MEMUPR_LOOP
                JMP     SHORT DOS_INT_2

MEMUPR_HIGH:    CALL    CD_CASEMAP
                JMP     SHORT MEMUPR_STORE

;
; Software interrupt 10H
;
                ASSUME  DS:SV_DATA
                TALIGN  4
VIO_INT:        STI                             ; Enable interrupts
                CALL    GET_REGS                ; Load registers from variables
                ASSUME  DS:NOTHING
                PUSH    AX                      ; Save AH (function number)
                INT     10H                     ; Call BIOS
                CALL    PUT_REGS                ; Store registers to variables
                POP     AX                      ; Restore AH (function number)
                CMP     AH, 00H                 ; Set mode?
                JNE     INT_BACK                ; No  -> back to protected mode
                CALL    VINIT                   ; Get new width and height
                JMP     INT_BACK                ; Back to protected mode

;
; Generic software interrupt
;
                ASSUME  DS:SV_DATA
                TALIGN  4
SOFTWARE_INT:   STI                             ; Enable interrupts
                MOV     CS:INT_OP_2+1, AL       ; Self-modifying code,
                JMP     FLUSH2                  ; flush prefetch queue!


;
; Load registers from variables
;
; Attention: ES:BP points to protected mode stack frame!
; 
                ASSUME  DS:NOTHING
                TALIGN  4
GET_REGS        PROC    NEAR
                MOV     AX, ES:RM_AX
                MOV     BX, ES:RM_BX
                MOV     CX, ES:RM_CX
                MOV     DX, ES:RM_DX
                MOV     SI, ES:RM_SI
                MOV     DI, ES:RM_DI
                MOV     DS, ES:RM_DS
                PUSH    ES:RM_BP
                MOV     ES, ES:RM_ES
                POP     BP
                RET
GET_REGS        ENDP


;
; Has to load ES:BP with address of protected-mode stack frame!
; 
                ASSUME  DS:NOTHING
                TALIGN  4
PUT_REGS        PROC    NEAR
                PUSH    DS
                PUSH    ES
                PUSH    BP
                PUSH    AX
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                MOV     AX, SV_STACK
                MOV     ES, AX
                MOV     BP, FRAME_PTR
                LAHF
                MOV     ES:RM_FLAGS, AH
                POP     ES:RM_AX
                POP     ES:RM_BP
                POP     ES:RM_ES
                POP     ES:RM_DS
                MOV     ES:RM_BX, BX
                MOV     ES:RM_CX, CX
                MOV     ES:RM_DX, DX
                MOV     ES:RM_SI, SI
                MOV     ES:RM_DI, DI
                RET
PUT_REGS        ENDP



;
; Allocate buffers for real-mode interface.
;
; We need two buffers:
;
;  - the first buffer is used for the user program's i/o
;
;  - the second buffer is used for the swapper's i/o. The swapper
;    may be activated while copying the user program's i/o buffer
;
; The two buffers have there own selectors (and GDT entries)
;

                ASSUME  DS:SV_DATA
INIT_BUFFER     PROC    NEAR
                MOV     BX, 1000H               ; 64 KB
                LEA     DI, G_BUF1_DESC
                CALL    INIT_BUF_1
                MOV     BUF1_SEG, AX            ; Main buffer segment
                MOV     BX, 0101H               ; 4 KB + 16 Bytes
                LEA     DI, G_BUF2_DESC
                CALL    INIT_BUF_1
                MOV     BUF2_SEG, AX            ; Swapper buffer segment
;
; Select 1st buffer (64KB)
;
                MOV     AX, BUF1_SEG
                MOV     BUF_SEG, AX
                MOV     BUF_SEL, G_BUF1_SEL
                RET


INIT_BUF_1      PROC    NEAR
                MOV     AH, 48H
                INT     21H
                JC      RM_OUT_OF_MEM
                PUSH    AX
                MOVZX   EAX, AX
                SHL     EAX, 4
                CALL    RM_SEG_BASE
                POP     AX
                RET
INIT_BUF_1      ENDP

INIT_BUFFER     ENDP


              IF DEBUG_DOSDUMP

                ASSUME  DS:NOTHING
DOSDUMP         PROC    NEAR
                PUSHAD
                PUSH    DS
                PUSH    ES
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                LEA     BX, $DOSDUMP_C
DOSDUMP_10:     MOV     AL, [BX]
                INC     BYTE PTR [BX]
                CMP     AL, "9"
                JNE     SHORT DOSDUMP_11
                MOV     BYTE PTR [BX], "0"
                DEC     BX
                JMP     SHORT DOSDUMP_10

DOSDUMP_11:     LEA     DX, $DOSDUMP
                MOV     CX, 0
                MOV     AH, 3CH
                INT     21H
                JC      SHORT DOSDUMP_ERR
                MOV     BX, AX
                MOV     CX, 640 / 32
                MOV     SI, 0
DOSDUMP_20:     PUSH    CX
                PUSH    DS
                MOV     DS, SI
                ASSUME  DS:NOTHING
                MOV     DX, 0
                MOV     CX, 32 * 1024
                MOV     AH, 40H
                INT     21H
                POP     DS
                ASSUME  DS:SV_DATA
                POP     CX
                JC      SHORT DOSDUMP_ERR
                ADD     SI, 32 * 1024 / 16
                LOOP    DOSDUMP_20
                MOV     AH, 3EH
                INT     21H
                JMP     SHORT DOSDUMP_RET

DOSDUMP_ERR:
DOSDUMP_RET:    POP     ES
                POP     DS
                POPAD
                RET
DOSDUMP         ENDP

              ENDIF


INIT_CODE       ENDS

                END
