; $Id: tst1asm.asm 678 2003-09-09 19:21:51Z bird $
;
; Optlink testcase no. 1.
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
	.387
CODE32	segment para use32 public 'CODE'
CODE32	ends
DATA32	segment para use32 public 'DATA'
DATA32	ends
CONST32_RO	segment para use32 public 'CONST'
CONST32_RO	ends
BSS32	segment para use32 public 'BSS'
BSS32	ends
DGROUP	group BSS32, DATA32
	assume	cs:FLAT, ds:FLAT, ss:FLAT, es:FLAT
CONST32_RO	segment
@CBE1	dq 3ff199999999999ar	; 1.1000000000000000e+00
@CBE2	dq 3ff3333333333333r	; 1.2000000000000000e+00
@CBE3	dq 3ff4cccccccccccdr	; 1.3000000000000000e+00
@CBE4	dq 3ff6666666666666r	; 1.4000000000000000e+00
CONST32_RO	ends
CODE32	segment

; 15 extern int _Optlink asmfoo (int i1, int i2, int i3, double rf1, double rf2, double rf3, double rf4)
	align 010h

	public asmfoo
asmfoo	proc
	push	ebp
	mov	ebp,esp
	mov	[ebp+08h],eax;	i1
	mov	[ebp+0ch],edx;	i2
	mov	[ebp+010h],ecx;	i3
	fstp	qword ptr [ebp+014h];	rf1
	fstp	qword ptr [ebp+01ch];	rf2
	fstp	qword ptr [ebp+024h];	rf3
	fstp	qword ptr [ebp+02ch];	rf4

; 17     if (i1 != 1)
	cmp	dword ptr [ebp+08h],01h;	i1
	je	@BLBL1

; 18         return 1;
	mov	eax,01h
	pop	ebp
	ret	
	align 010h
@BLBL1:

; 19     if (i2 != 2)
	cmp	dword ptr [ebp+0ch],02h;	i2
	je	@BLBL2

; 20         return 2;
	mov	eax,02h
	pop	ebp
	ret	
	align 010h
@BLBL2:

; 21     if (i3 != 3)
	cmp	dword ptr [ebp+010h],03h;	i3
	je	@BLBL3

; 22         return 3;
	mov	eax,03h
	pop	ebp
	ret	
	align 010h
@BLBL3:

; 23     if (rf1 != 1.1)
	fld	qword ptr  @CBE1
	fld	qword ptr [ebp+014h];	rf1
	fucompp	
	fnstsw	ax
	and	ax,04500h
	cmp	ax,04000h
	je	@BLBL4

; 24         return 4;
	mov	eax,04h
	pop	ebp
	ret	
	align 010h
@BLBL4:

; 25     if (rf2 != 1.2)
	fld	qword ptr  @CBE2
	fld	qword ptr [ebp+01ch];	rf2
	fucompp	
	fnstsw	ax
	and	ax,04500h
	cmp	ax,04000h
	je	@BLBL5

; 26         return 5;
	mov	eax,05h
	pop	ebp
	ret	
	align 010h
@BLBL5:

; 27     if (rf3 != 1.3)
	fld	qword ptr  @CBE3
	fld	qword ptr [ebp+024h];	rf3
	fucompp	
	fnstsw	ax
	and	ax,04500h
	cmp	ax,04000h
	je	@BLBL6

; 28         return 6;
	mov	eax,06h
	pop	ebp
	ret	
	align 010h
@BLBL6:

; 29     if (rf4 != 1.4)
	fld	qword ptr  @CBE4
	fld	qword ptr [ebp+02ch];	rf4
	fucompp	
	fnstsw	ax
	and	ax,04500h
	cmp	ax,04000h
	je	@BLBL7

; 30         return 7;
	mov	eax,07h
	pop	ebp
	ret	
	align 010h
@BLBL7:

; 31     return 0;
	mov	eax,0h
	pop	ebp
	ret	
asmfoo	endp
CODE32	ends
end
