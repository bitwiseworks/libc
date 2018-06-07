;
; XMS.INC -- Manage extended memory
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

SV_DATA         SEGMENT

                EXTRN   XMS_FLAG:BYTE
                EXTRN   DISABLE_XMS_MEM:BYTE

SV_DATA         ENDS

INIT_CODE       SEGMENT

                EXTRN   CHECK_XMS:NEAR, INIT_XMS:NEAR, CLEANUP_XMS:NEAR
                EXTRN   XMS_A20_ON:NEAR, XMS_A20_OFF:NEAR

INIT_CODE       ENDS
