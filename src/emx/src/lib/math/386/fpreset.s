/ fpreset.s (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __fpreset

        .text

        ALIGN

/ void _fpreset (void)

__fpreset:
        PROFILE_NOFRAME
        finit
        EPILOGUE(__fpreset)
