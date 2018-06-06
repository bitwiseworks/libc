; $Id: ldstub.asm 214 2003-05-21 22:34:46Z zap $
;
; Micro stub for <app> -> <app>.exe forwarding
;
; Build Instructions:
;   Stantard version:
;       alp ldstub.asm
;       ilink /pmtype:vio /ALIGN:1 /OUT:ldstub.bin ldstub.obj ldstub.def os2386.lib
;
;   Microversion:
;       alp -D:MICRO=1 ldstub.asm
;       ilink /pmtype:vio /ALIGN:1 /OUT:ldstub.bin ldstub.obj ldstub.def os2386.lib
;
; OVERLAY may be defined for execv()/spawnv(OVERLAY,..) is to be emulated.
;
; InnoTek Systemberatung GmbH Confidential
;
; Copyright (c) 2003 InnoTek Systemberatung GmbH
; Author: knut st. osmundsen <bird@anduin.net>
;
; All Rights Reserved
;
;

    .486

;*******************************************************************************
;* Structures and Typedefs                                                     *
;*******************************************************************************
RESULTCODES	STRUC
resc_codeTerminate	DD	?
resc_codeResult	DD	?
RESULTCODES	ENDS

extrn DosExecPgm:near
extrn DosQueryModuleName:near


DATA32 segment use32 para public 'DATA'
DATA32 ends

BSS32	segment use32 para public 'BSS'
; buffer for modifying the executable name
achNewExeName   db 260 dup(?)
padding         db   8 dup(?)           ; safety precaution.

; DosExecPgm result buffer.
res             RESULTCODES <?,?>
BSS32	ends

STACK32 segment use32 para stack 'STACK'
    dd  1800h dup(?)                    ; size doesn't matter as DosExecPgm uses thunk stack in any case. :/
STACK32 ends

ifndef MICRO
DGROUP group DATA32, BSS32, STACK32
else
DGROUP group CODE32, DATA32, BSS32, STACK32
endif


ifndef MICRO
CODE32 segment use32 para public 'CODE'
else
CODE32 segment use32 para public 'DATA'
endif

; Program startup registers are defined as follows. 
;    EIP = Starting program entry address. 
;    ESP = Top of stack address. 
;    CS = Code selector for base of linear address space. 
;    DS = ES = SS = Data selector for base of linear address space. 
;    FS = Data selector of base of Thread Information Block (TIB). 
;    GS = 0. 
;    EAX = EBX = 0. 
;    ECX = EDX = 0. 
;    ESI = EDI = 0. 
;    EBP = 0.   
;    [ESP+0] = Return address to routine which calls DosExit(1,EAX). 
;    [ESP+4] = Module handle for program module. 
;    [ESP+8] = Reserved. 
;    [ESP+12] = Environment data object address. 
;    [ESP+16] = Command line linear address in environment data object. 
;
; @remark   I don't care about cleaning up arguments as the leave does so.
;
start:
    ASSUME ss:FLAT, ds:FLAT, es:FLAT, fs:nothing, gs:nothing
    push    ebp        
    mov     ebp, esp
    
    ;
    ; Get the executable name.
    ;
    ; ULONG DosQueryModuleName(HMODULE hmod, ULONG ulNameLength, PCHAR pNameBuf);
    ;   
    push    offset FLAT:achNewExeName   ; pNameBuf
    push    size achNewExeName          ; ulNameLength
    push    dword ptr [ebp+8]           ; hmod
    call    DosQueryModuleName
    or      eax, eax
    jz      modname_ok
    
    ; this ain't supposed to happen
    mov     edx, [ebp+4]
    int     3    

modname_ok:
    ;
    ; Append .EXE to the module name.
    ;
    xor     ecx, ecx
    dec     ecx
    mov     edi, offset FLAT:achNewExeName
    repne scasb
    mov     dword ptr [edi-1], 'EXE.'
    mov     dword ptr [edi+3], 0

    ;
    ; Try execute the .EXE appended module name.
    ;
    ; APIRET APIENTRY DosExecPgm(PCHAR pObjname,
    ;                            LONG cbObjname,
    ;                            ULONG execFlag,
    ;                            PCSZ  pArg,
    ;                            PCSZ  pEnv,
    ;                            PRESULTCODES pRes,
    ;                            PCSZ  pName);
    ;
    xor     ecx, ecx
    push    offset FLAT:achNewExeName   ; pName
    push    offset FLAT:res             ; pRes
    push    dword ptr [ebp + 10h]       ; pEnv
    push    dword ptr [ebp + 14h]       ; pArg
ifdef OVERLAY
    push    1                           ; execFlag = EXEC_ASYNC
else
    push    ecx                         ; execFlag = EXEC_SYNC = 0
endif    
    push    ecx                         ; cbObjname
    push    ecx                         ; pObjname
    call    DosExecPgm
    or      eax, eax
    jz      exec_success
    
exec_failed:
    mov     eax, 07fh
    jmp     done
    
exec_success:        
ifndef OVERLAY
    ;
    ; Retrieve the result code.
    ;
    mov     eax, res.resc_codeResult
    mov     edx, res.resc_codeTerminate
    or      edx, edx
    jz      done
    
    ;
    ; Something bad happend and the result code is 0.
    ; We shouldn't return 0 when we trap, crash or is killed.
    ; 
    mov     eax, 0ffh
endif            


done:
    leave   
    ret     
CODE32 ends

end start

