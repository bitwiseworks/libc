/ binmode.s (emx+gcc) -- Copyright (c) 1992-1994 by Eberhard Mattes

        .text

L_setbinmode:
        movl    $1, __fmode_bin
        ret

        .stabs  "___CTOR_LIST__", 23, 0, 0, L_setbinmode
