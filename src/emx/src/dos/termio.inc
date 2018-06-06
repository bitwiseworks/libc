;
; TERMIO.INC -- General terminal interface
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

;
; cf. /emx/include/sys/termio.h and /emx/include/sys/ioctl.h
;

TCGETA          =       1
TCSETA          =       2
TCSETAW         =       3
TCSETAF         =       4
TCFLSH          =       5
TCSBRK          =       6
TCXONC          =       7
_TCGA           =       8               ; Used internally for tcgetattr()
_TCSANOW        =       9               ; Used internally for tcsetattr()
_TCSADRAIN      =       10              ; Used internally for tcsetattr()
_TCSAFLUSH      =       11              ; Used internally for tcsetattr()
FIONREAD        =       16
FGETHTYPE       =       32

VINTR           =       0
VQUIT           =       1
VERASE          =       2
VKILL           =       3
VEOF            =       4
VEOL            =       5
VMIN            =       6
VTIME           =       7
VSUSP           =       8                       ; termios
VSTOP           =       9                       ; termios
VSTART          =       10                      ; termios

NCC             =       8                       ; termio
NCCS            =       11                      ; termios

IGNBRK          =       0001H
BRKINT          =       0002H
IGNPAR          =       0004H
PARMRK          =       0008H
INPCK           =       0010H
ISTRIP          =       0020H
INLCR           =       0040H
IGNCR           =       0080H
ICRNL           =       0100H
IUCLC           =       0200H
IXON            =       0400H
IXANY           =       0800H
IXOFF           =       1000H
IDELETE         =       8000H

OPOST           =       0001H
OLCUC           =       0002H
ONLCR           =       0004H
OCRNL           =       0008H
ONOCR           =       0010H
ONLRET          =       0020H
OFILL           =       0040H
OFDEL           =       0080H
NLDLY           =       0100H   ; Mask
NL0             =       0000H
NL1             =       0100H
CRDLY           =       0600H   ; Mask
_CR0            =       0000H
_CR1            =       0200H
_CR2            =       0400H
_CR3            =       0600H
TABDLY          =       1800H   ; Mask
TAB0            =       0000H
TAB1            =       0800H
TAB2            =       1000H
TAB3            =       1800H
BSDLY           =       2000H   ; Mask
BS0             =       0000H
BS1             =       2000H
VTDLY           =       4000H   ; Mask
VT0             =       0000H
VT1             =       4000H
FFDLY           =       8000H   ; Mask
FF0             =       0000H
FF1             =       8000H

CBAUD           =       000FH   ; Mask
B0              =       0000H
B50             =       0001H
B75             =       0002H
B110            =       0003H
B134            =       0004H
B150            =       0005H
B200            =       0006H
B300            =       0007H
B600            =       0008H
B1200           =       0009H
B1800           =       000AH
B2400           =       000BH
B4800           =       000CH
B9600           =       000DH
B19200          =       000EH
B38400          =       000FH
CSIZE           =       0030H   ; Mask
CS5             =       0000H
CS6             =       0010H
CS7             =       0020H
CS8             =       0030H
CSTOPB          =       0040H
CREAD           =       0080H
PARENB          =       0100H
PARODD          =       0200H
HUPCL           =       0400H
CLOCAL          =       0800H
LOBLK           =       1000H

ISIG            =       0001H
ICANON          =       0002H
XCASE           =       0004H
IECHO           =       0008H
ECHOE           =       0010H
ECHOK           =       0020H
ECHONL          =       0040H
NOFLSH          =       0080H
IDEFAULT        =       8000H


;
; ioctl()
;
TERMIO          STRUCT
C_IFLAG         DD      ?
C_OFLAG         DD      ?
C_CFLAG         DD      ?
C_LFLAG         DD      ?
C_LINE          DD      ?
C_CC            DB      NCC DUP (?)
TERMIO          ENDS

TERMIOS         STRUCT
C_IFLAG         DD      ?
C_OFLAG         DD      ?
C_CFLAG         DD      ?
C_LFLAG         DD      ?
C_CC            DB      NCCS DUP (?)
                ALIGN   4
C_RESERVED      DD      4 DUP (?)
TERMIOS         ENDS

MY_TERMIO       STRUCT
C_IFLAG         DD      ?
C_OFLAG         DD      ?
C_CFLAG         DD      ?
C_LFLAG         DD      ?
C_CC            DB      NCCS DUP (?)
MY_TERMIO       ENDS

;
; cf. /emx/include/sys/fcntl.h
;
F_GETFL         =       1
F_SETFL         =       2
F_GETFD         =       3
F_SETFD         =       4

O_NDELAY        =       04H
O_APPEND        =       08H

              IFNDEF __TERMIO

SV_DATA         SEGMENT

                EXTRN   STDIN_TERMIO:MY_TERMIO
                EXTRN   STDIN_FL:DWORD

SV_DATA         ENDS


SV_CODE         SEGMENT

                EXTRN   TERMIO_INIT:NEAR
                EXTRN   TERMIO_READ:NEAR
                EXTRN   STDIN_AVAIL:NEAR
                EXTRN   KBD_FLUSH:NEAR
                EXTRN   TERMIO_GET:NEAR
                EXTRN   TERMIO_SET:NEAR
                EXTRN   TERMIO_FLUSH:NEAR
                EXTRN   TERMIOS_GET:NEAR
                EXTRN   TERMIOS_SET:NEAR
                EXTRN   TERMIOS_FLUSH:NEAR

SV_CODE         ENDS

INIT_CODE       SEGMENT

                EXTRN   POLL_KEYBOARD:NEAR
                EXTRN   INIT_TERMIO:NEAR

INIT_CODE       ENDS

              ENDIF
