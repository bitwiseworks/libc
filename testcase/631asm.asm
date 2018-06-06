	title	631asmc.c
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
CODE32	segment

; 107 struct ret16bytes __stdcall asmfoostd16(void)
	align 010h

	public _asmfoostd16@0
_asmfoostd16@0	proc
	push	ebp
	mov	ebp,esp

; 110     return ret;
	mov	eax,[ebp+08h];	@CBE10

; 109     struct ret16bytes ret = {1,2,3,4};
	mov	dword ptr [ebp-010h],01h;	ret
	mov	dword ptr [ebp-0ch],02h;	ret

; 110     return ret;
	mov	ecx,[ebp-010h];	ret
	mov	edx,[ebp-0ch];	ret
	mov	[eax],ecx

; 109     struct ret16bytes ret = {1,2,3,4};
	mov	dword ptr [ebp-08h],03h;	ret

; 110     return ret;
	mov	[eax+04h],edx
	mov	edx,[ebp-08h];	ret

; 109     struct ret16bytes ret = {1,2,3,4};
	mov	dword ptr [ebp-04h],04h;	ret

; 110     return ret;
	mov	ecx,[ebp-04h];	ret
	mov	[eax+08h],edx
	mov	[eax+0ch],ecx
	pop	ebp
	ret	04h
_asmfoostd16@0	endp

; 101 struct ret12bytes __stdcall asmfoostd12(void)
	align 010h

	public _asmfoostd12@0
_asmfoostd12@0	proc

; 104     return ret;
	mov	eax,[esp+04h];	@CBE9

; 103     struct ret12bytes ret = {1,2,3};
	mov	dword ptr [esp-0ch],01h;	ret
	mov	dword ptr [esp-08h],02h;	ret

; 104     return ret;
	mov	edx,[esp-0ch];	ret
	mov	ecx,[esp-08h];	ret
	mov	[eax],edx

; 103     struct ret12bytes ret = {1,2,3};
	mov	dword ptr [esp-04h],03h;	ret

; 104     return ret;
	mov	[eax+04h],ecx
	mov	ecx,[esp-04h];	ret
	mov	[eax+08h],ecx
	ret	04h
_asmfoostd12@0	endp

; 95 struct ret8bytes __stdcall  asmfoostd8(void)
	align 010h

	public _asmfoostd8@0
_asmfoostd8@0	proc

; 97     struct ret8bytes ret = {1,2};
	mov	dword ptr [esp-08h],01h;	ret
	mov	dword ptr [esp-04h],02h;	ret

; 98     return ret;
	mov	eax,[esp-08h];	ret
	mov	edx,[esp-04h];	ret
	ret	
_asmfoostd8@0	endp

; 89 struct ret4bytes __stdcall  asmfoostd4(void)
	align 010h

	public _asmfoostd4@0
_asmfoostd4@0	proc

; 91     struct ret4bytes ret = {1};
	mov	dword ptr [esp-04h],01h;	ret

; 92     return ret;
	mov	eax,[esp-04h];	ret
	ret	
_asmfoostd4@0	endp

; 81 struct ret16bytes _Optlink  asmfooopt16(void)
	align 010h

	public asmfooopt16
asmfooopt16	proc
	push	ebp
	mov	ebp,esp

; 84     return ret;
	mov	eax,[ebp+08h];	@CBE8

; 83     struct ret16bytes ret = {1,2,3,4};
	mov	dword ptr [ebp-010h],01h;	ret
	mov	dword ptr [ebp-0ch],02h;	ret

; 84     return ret;
	mov	ecx,[ebp-010h];	ret
	mov	edx,[ebp-0ch];	ret
	mov	[eax],ecx

; 83     struct ret16bytes ret = {1,2,3,4};
	mov	dword ptr [ebp-08h],03h;	ret

; 84     return ret;
	mov	[eax+04h],edx
	mov	edx,[ebp-08h];	ret

; 83     struct ret16bytes ret = {1,2,3,4};
	mov	dword ptr [ebp-04h],04h;	ret

; 84     return ret;
	mov	ecx,[ebp-04h];	ret
	mov	[eax+08h],edx
	mov	[eax+0ch],ecx
	pop	ebp
	ret	
asmfooopt16	endp

; 75 struct ret12bytes _Optlink  asmfooopt12(void)
	align 010h

	public asmfooopt12
asmfooopt12	proc

; 78     return ret;
	mov	eax,[esp+04h];	@CBE7

; 77     struct ret12bytes ret = {1,2,3};
	mov	dword ptr [esp-0ch],01h;	ret
	mov	dword ptr [esp-08h],02h;	ret

; 78     return ret;
	mov	edx,[esp-0ch];	ret
	mov	ecx,[esp-08h];	ret
	mov	[eax],edx

; 77     struct ret12bytes ret = {1,2,3};
	mov	dword ptr [esp-04h],03h;	ret

; 78     return ret;
	mov	[eax+04h],ecx
	mov	ecx,[esp-04h];	ret
	mov	[eax+08h],ecx
	ret	
asmfooopt12	endp

; 69 struct ret8bytes _Optlink   asmfooopt8(void)
	align 010h

	public asmfooopt8
asmfooopt8	proc

; 72     return ret;
	mov	eax,[esp+04h];	@CBE6

; 71     struct ret8bytes ret = {1,2};
	mov	dword ptr [esp-08h],01h;	ret
	mov	dword ptr [esp-04h],02h;	ret

; 72     return ret;
	mov	edx,[esp-08h];	ret
	mov	ecx,[esp-04h];	ret
	mov	[eax],edx
	mov	[eax+04h],ecx
	ret	
asmfooopt8	endp

; 63 struct ret4bytes _Optlink   asmfooopt4(void)
	align 010h

	public asmfooopt4
asmfooopt4	proc

; 66     return ret;
	mov	eax,[esp+04h];	@CBE5

; 65     struct ret4bytes ret = {1};
	mov	dword ptr [esp-04h],01h;	ret

; 66     return ret;
	mov	ecx,[esp-04h];	ret
	mov	[eax],ecx
	ret	
asmfooopt4	endp

; 56 struct ret16bytes _System   asmfoosys16(void)
	align 010h

	public asmfoosys16
asmfoosys16	proc
	push	ebp
	mov	ebp,esp

; 59     return ret;
	mov	eax,[ebp+08h];	@CBE4

; 58     struct ret16bytes ret = {1,2,3,4};
	mov	dword ptr [ebp-010h],01h;	ret
	mov	dword ptr [ebp-0ch],02h;	ret

; 59     return ret;
	mov	ecx,[ebp-010h];	ret
	mov	edx,[ebp-0ch];	ret
	mov	[eax],ecx

; 58     struct ret16bytes ret = {1,2,3,4};
	mov	dword ptr [ebp-08h],03h;	ret

; 59     return ret;
	mov	[eax+04h],edx
	mov	edx,[ebp-08h];	ret

; 58     struct ret16bytes ret = {1,2,3,4};
	mov	dword ptr [ebp-04h],04h;	ret

; 59     return ret;
	mov	ecx,[ebp-04h];	ret
	mov	[eax+08h],edx
	mov	[eax+0ch],ecx
	pop	ebp
	ret	
asmfoosys16	endp

; 50 struct ret12bytes _System   asmfoosys12(void)
	align 010h

	public asmfoosys12
asmfoosys12	proc

; 53     return ret;
	mov	eax,[esp+04h];	@CBE3

; 52     struct ret12bytes ret = {1,2,3};
	mov	dword ptr [esp-0ch],01h;	ret
	mov	dword ptr [esp-08h],02h;	ret

; 53     return ret;
	mov	edx,[esp-0ch];	ret
	mov	ecx,[esp-08h];	ret
	mov	[eax],edx

; 52     struct ret12bytes ret = {1,2,3};
	mov	dword ptr [esp-04h],03h;	ret

; 53     return ret;
	mov	[eax+04h],ecx
	mov	ecx,[esp-04h];	ret
	mov	[eax+08h],ecx
	ret	
asmfoosys12	endp

; 44 struct ret8bytes _System    asmfoosys8(void)
	align 010h

	public asmfoosys8
asmfoosys8	proc

; 47     return ret;
	mov	eax,[esp+04h];	@CBE2

; 46     struct ret8bytes ret = {1,2};
	mov	dword ptr [esp-08h],01h;	ret
	mov	dword ptr [esp-04h],02h;	ret

; 47     return ret;
	mov	edx,[esp-08h];	ret
	mov	ecx,[esp-04h];	ret
	mov	[eax],edx
	mov	[eax+04h],ecx
	ret	
asmfoosys8	endp

; 38 struct ret4bytes _System    asmfoosys4(void)
	align 010h

	public asmfoosys4
asmfoosys4	proc

; 41     return ret;
	mov	eax,[esp+04h];	@CBE1

; 40     struct ret4bytes ret = {1};
	mov	dword ptr [esp-04h],01h;	ret

; 41     return ret;
	mov	ecx,[esp-04h];	ret
	mov	[eax],ecx
	ret	
asmfoosys4	endp




public _asmfoodef4_gcc
_asmfoodef4_gcc:
	mov	    eax, 1
	ret
    
public _asmfoodef8_gcc
_asmfoodef8_gcc:
	mov	    eax, 1
	mov	    edx, 2
	ret
    
public _asmfoodef12_gcc
_asmfoodef12_gcc:
	push 	ebp
	mov 	ebp, esp
	sub 	esp, 24
	mov 	eax, [ebp + 8]
	mov 	dword ptr [eax + 0], 1
	mov 	dword ptr [eax + 4], 2
	mov 	dword ptr [eax + 8], 3
	leave
	ret	4
    
public _asmfoodef16_gcc
_asmfoodef16_gcc:
	push	ebp
	mov 	ebp, esp
	sub 	esp, 24
	mov 	eax, [ebp + 8]
	mov 	dword ptr [eax + 0], 1
	mov 	dword ptr [eax + 4], 2
	mov 	dword ptr [eax + 8], 3
	mov 	dword ptr [eax +12], 4
	leave
	ret	4

CODE32	ends
end
