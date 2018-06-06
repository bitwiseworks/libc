; $Id: empty-ref.asm 405 2003-07-17 01:19:17Z bird $
;
; Reference program, I can't do anything faster than this.
;
; InnoTek Systemberatung GmbH confidential
;
; Copyright (c) 2003 InnoTek Systemberatung GmbH
; Author: knut st. osmundsen <bird-srcspam@anduin.net>
;
; All Rights Reserved
;
;
    .386

STACK32 segment use32 stack para 'STACK'
    db 4096 dup(0)
STACK32 ends

CODE32 segment use32 public para 'CODE'
    ASSUME ds:FLAT, cs:FLAT, ss:FLAT
start:
    xor     eax, eax
    ret
CODE32 ends

end start

