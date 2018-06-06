/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 *
 * Adapted for `long double' by Ulrich Drepper <drepper@cygnus.com>.
 */

//#include <machine/asm.h>
#include <sysdep.h>
#include <asm-syntax.h>

//RCSID("$NetBSD: $")

ENTRY(_STD(remainderl))
	fldt	16(%esp)
	fldt	4(%esp)
1:	fprem1
	fstsw	%ax
	sahf
	jp	1b
	fstp	%st(1)
	ret
END (_STD(remainderl))
