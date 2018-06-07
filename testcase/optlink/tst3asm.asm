; $Id: tst3asm.asm 633 2003-08-17 18:41:32Z bird $
;
; Testing calling conventions and name decoration (or lack of such).
;
;
; Copyright (c) 2003 InnoTek Systemberatung GmbH
; Author: knut st. osmundsen <bird@anduin.net>
;
; Project Odin Software License can be found in LICENSE.TXT.
;
;
    .386


CODE32 segment use32 para public 'CODE'

extrn OptSimple:near
extrn OptPrintf:near
extrn _vsprintf:near


public asmfunc
asmfunc proc near
    xor eax, eax
    ret
asmfunc endp

;;
; asmsprintf(char *pszBuffer, const char* pszFormat, ...);
;
public asmsprintf
asmsprintf proc near
    push    ebp
    mov     ebp, esp

    lea     ecx, [ebp+010h]             ; &...
    push    ecx
    push    edx                         ; pszFormat
    push    eax                         ; pszBuffer
    call    _vsprintf

    leave
    ret
asmsprintf endp

CODE32 ends


end
