/ _errno.s (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes

#include <emx/asm386.h>

	.globl  __errno
	.globl  __errno_fun

    .data
initterm_errno:
    .long  0

	.text
	ALIGN

__errno:
__errno_fun:
    movl    ___libc_gpTLS, %eax
    orl     %eax, %eax
    jnz     ok

    /* ignore calls before TLS inits. (fork registration failure) */
    movl    $initterm_errno, %eax
    jmp     done
    ALIGN
ok:
    movl    (%eax), %eax
    orl     %eax, %eax
    jnz     done
    call    ___libc_threadCurrentSlow
    jmp     done

    ALIGN
done:
	EPILOGUE(_errno)
