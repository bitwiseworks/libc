/ thunk1.asm (emx+gcc) -- Copyright (c) 1992-1994 by Eberhard Mattes

		.globl	__libc_thunk1

/
/ unsigned long _libc_thunk1 (void *args, void *fun)
/
/ Call 16-bit code
/
/ In:   ARGS    Pointer to argument list. The first DWORD contains the
/               number of argument bytes, excluding that DWORD. The
/               remaining values are packed appropriately for calling
/               a 16-bit function. Pointers have been converted to
/               sel:offset format
/
/       FUN     16:16 address of 16-bit function to be called. Both
/               `pascal' and `cdecl' calling conventions are supported.
/               The function must not change the DI register
/
/ Out:  EAX     Return value (DX:AX) of 16-bit function
/
		.p2align 2

		.set	ARGS, 4*4		# Offset to ARGS relative to %ebp
		.set	FUN, 5*4		# Offset to FUN relative to %ebp
		.set	RETADDR, -4*4		# Offset to spare space relative to FUN

__libc_thunk1:	pushl	%ebp			# Leave two dwords spare
		pushl	%ebp			# space for 32-bit return address
		pushl	%ebp			# Set up stack frame
		movl	%esp, %ebp
		pushl	%esi			# Save %esi
		pushl	%edi			# Save %edi
		pushl	%ebx			# Save %ebx
		pushl	%es			# (2) Save %es
		movl	%esp, %eax		# Check	stack
		cmpw	$0x1000, %ax 		# 1000H	bytes left in this 64K
		jae	1f			# segment? Yes => skip
		xorw	%ax, %ax		# Move %esp down to next 64K seg
		movb	$0, (%eax)		# Stack	probe
		xchgl	%esp, %eax		# Set new %esp, %eax := old %esp
1:		pushl	%ss			# Save original	%ss:%esp on
		pushl	%eax			# stack	(points	to saved %ebx)
/
/ Copy arguments
/
		movl	ARGS(%ebp), %esi
		lodsl
		movl	%eax, %ecx
		subl	%ecx, %esp
		movl	%esp, %edi
		shrl	$2, %ecx
		repne
		movsl
		movl	%eax, %ecx
		andl	$3, %ecx
		repne
		movsb				# %edi now points to %ss:%esp
		leal	FUN(%ebp), %esi
/
/ Convert %eip to %cs:%ip and %esp to %ss:%sp
/
		movw	%cs, RETADDR+4(%esi)
		movl	$Lthunk1_ret, RETADDR(%esi)

		movl	$Lthunk16_call, %eax
		call	DosFlatToSel
		movzwl	%ax, %ecx		# %ecx = offset
		shrl	$16, %eax		# %eax = segment
		pushl	%eax
		pushl	%ecx

		movl	%esp, %eax
		call	DosFlatToSel
		movzwl	%ax, %ecx		# %ecx = offset
		shrl	$16, %eax		# %eax = segment
		pushl	%eax
		pushl	%ecx

/
/ Jump to 16-bit code
/
		lss	(%esp), %esp		# Switch to new	%ss:%sp
		lret

/
/ Call 16-bit function
/
/ In:	 %esi	Points to 16:16	function address
/ Note:  Actually this is 16-bit code. It is put inside the 32-bit code
/        since a.out format does not support 16-bit segments.
/
		.p2align 2
Lthunk16_call:
/		lcall	*(%esi)
		.byte	0x67,0xff,0x1e
/		ljmp	*RETADDR(%esi)
		.byte	0x67,0x66,0xff,0x6e,RETADDR

		.p2align 2
Lthunk1_ret:	movzwl	%di, %esp		# (4) Remove arguments
		lss	(%esp),%esp		# Get 32-bit stack pointer
		popl	%es			# Restore %es
		popl	%ebx			# Restore %ebx
		popl	%edi			# Restore %edi
		popl	%esi			# Restore %esi
		popl	%ebp			# Restore %ebp
		addl	$4*2,%esp		# Skip spare space
		movzwl	%ax, %eax		# Compute return value
		movzwl	%dx, %edx
		shll	$16, %edx
		orl	%edx, %eax
		ret				# Return to 32-bit code
