/ thunk0.asm (emx+gcc) -- Copyright (c) 1992-1993 by Eberhard Mattes

		.globl  __libc_16to32
                .globl  __libc_32to16

		.p2align 2
__libc_32to16:	movl	4(%esp),%eax
		jmp	DosFlatToSel

		.p2align 2
__libc_16to32:	movl	4(%esp),%eax
		jmp	DosSelToFlat
