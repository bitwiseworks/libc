/*
 * Based on code written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>
__FBSDID("$FreeBSD: src/lib/msun/i387/s_truncf.S,v 1.1 2005/04/16 21:12:55 das Exp $")

STDENTRY(truncf)
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp

	fstcw	-12(%ebp)		/* store fpu control word */
	movw	-12(%ebp),%dx
	orw	$0x0c00,%dx		/* round towards -oo */
	movw	%dx,-16(%ebp)
	fldcw	-16(%ebp)		/* load modfied control word */

	flds	8(%ebp)			/* round */
	frndint

	fldcw	-12(%ebp)		/* restore original control word */

	leave
	ret
