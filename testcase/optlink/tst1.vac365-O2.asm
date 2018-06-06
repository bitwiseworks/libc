	extrn	_exeentry:dword
	extrn	_fltused:dword
	title	tst1.c
	.386
	.387
	includelib cpprss36.lib
	includelib os2386.lib
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

; 1 void foo (int i1, int i2, int i3, float rf1, float rf2, float rf3, float rf4)
	align 010h

	public foo
foo	proc
	fstp	st(0)
	fstp	st(0)
	fstp	st(0)
	fstp	st(0)

; 4 }
	ret	
foo	endp

; 6 int main()
	align 010h

	public main
main	proc

; 8     foo(1, 2, 3, 1.1, 1.2, 1.3, 1.4);
	fld	dword ptr  @CBE8
	fld	dword ptr  @CBE9
	fld	dword ptr  @CBE10
	fld	dword ptr  @CBE11
	mov	ecx,03h

; 6 int main()
	sub	esp,01ch

; 8     foo(1, 2, 3, 1.1, 1.2, 1.3, 1.4);
	mov	edx,02h
	mov	eax,01h
	call	foo

; 9     return 0;
	xor	eax,eax
	add	esp,01ch
	ret	
main	endp
CONST32_RO	segment
	align 04h
@CBE8	dd 3fb33333r	; 1.4000000e+00
	align 04h
@CBE9	dd 3fa66666r	; 1.3000000e+00
	align 04h
@CBE10	dd 3f99999ar	; 1.2000000e+00
	align 04h
@CBE11	dd 3f8ccccdr	; 1.1000000e+00
CONST32_RO	ends
CODE32	ends
end
