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
@CBE1	dq 3ff8000000000000r	; 1.5000000000000000e+00
CONST32_RO	ends
CODE32	segment

; 21 int _Optlink asmfoo(int i1, struct sss s1, void *pv, float rf1, struct sss s2)
	align 010h

	public asmfoo
asmfoo	proc
	push	ebp
	mov	ebp,esp
	mov	[ebp+08h],eax;	i1
	mov	[ebp+014h],edx;	pv
	fstp	dword ptr [ebp+018h];	rf1

; 23     if (i1 != 1)
	cmp	dword ptr [ebp+08h],01h;	i1
	je	@BLBL1

; 24         return 1;
	mov	eax,01h
	pop	ebp
	ret	
	align 010h
@BLBL1:

; 25     if (s1.a != 2)
	cmp	dword ptr [ebp+0ch],02h;	s1
	je	@BLBL2

; 26         return 2;
	mov	eax,02h
	pop	ebp
	ret	
	align 010h
@BLBL2:

; 27     if (s1.b != 3)
	cmp	dword ptr [ebp+010h],03h;	s1
	je	@BLBL3

; 28         return 3;
	mov	eax,03h
	pop	ebp
	ret	
	align 010h
@BLBL3:

; 29     if (pv != (void*)4)
	cmp	dword ptr [ebp+014h],04h;	pv
	je	@BLBL4

; 30         return 4;
	mov	eax,04h
	pop	ebp
	ret	
	align 010h
@BLBL4:

; 31     if (rf1 != (float)1.5)
	fld	dword ptr  @CBE2
	fld	dword ptr [ebp+018h];	rf1
	fucompp	
	fnstsw	ax
	and	ax,04500h
	cmp	ax,04000h
	je	@BLBL5

; 32         return 5;
	mov	eax,05h
	pop	ebp
	ret	
	align 010h
@BLBL5:

; 33     if (s2.a != 6)
	cmp	dword ptr [ebp+01ch],06h;	s2
	je	@BLBL6

; 34         return 6;
	mov	eax,06h
	pop	ebp
	ret	
	align 010h
@BLBL6:

; 35     if (s2.b != 7)
	cmp	dword ptr [ebp+020h],07h;	s2
	je	@BLBL7

; 36         return 7;
	mov	eax,07h
	pop	ebp
	ret	
	align 010h
@BLBL7:

; 37     return 0;
	mov	eax,0h
	pop	ebp
	ret	
asmfoo	endp
CONST32_RO	segment
	align 04h
@CBE2	dd 3fc00000r	; 1.5000000e+00
CONST32_RO	ends
CODE32	ends
end
