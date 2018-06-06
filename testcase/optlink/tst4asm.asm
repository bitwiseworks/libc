; $Id: tst4asm.asm 642 2003-08-19 00:26:01Z bird $
;
; Optlink testcase no. 4.
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

CODE32  segment para use32 public 'CODE'

; 15 int __cdecl asmdefaultconvention(int i1, int i2, int i3)
    public _asmdefaultconvention
_asmdefaultconvention   proc
    ;
    ; If all registers match it's suspicious!
    ;
    cmp     eax, 1
    jne     ok
    cmp     edx, 2
    jne     ok
    cmp     ecx, 3
    jne     ok
    ; they all match fail.
    mov     eax, 4
    ret

    ;
    ; Verify the stuff on the stack.
    ;
ok:
    cmp dword ptr [esp+04h],01h;    i1
    je      @BLBL1
    mov     eax,01h
    ret

@BLBL1:
    cmp dword ptr [esp+08h],02h;    i2
    je      @BLBL2
    mov     eax,02h
    ret

@BLBL2:
    xor     eax,eax
    cmp dword ptr [esp+0ch],03h;    i3
    setne   al
    neg     eax
    and     eax,03h
    ret
_asmdefaultconvention   endp

CODE32  ends

end
