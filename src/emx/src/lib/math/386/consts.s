/ consts.s (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes

        .globl  __const_NAN
        .globl  __const_ONE
        .globl  __const_ZERO
        .globl  __const_TWO
        .globl  __const_HALF
        .globl  __const_M_ONE
        .globl  __HUGE_VAL
        .globl  __LHUGE_VAL

__const_NAN:    .long   0xffffffff, 0x7fffffff
__const_ONE:    .long   0x00000000, 0x3ff00000
__const_ZERO:   .long   0x00000000, 0x00000000
__const_TWO:    .long   0x00000000, 0x40000000
__const_HALF:   .long   0x00000000, 0x3fe00000
__const_M_ONE:  .long   0x00000000, 0xbff00000

__HUGE_VAL:     .long   0x00000000, 0x7ff00000
__LHUGE_VAL:    .long   0x00000000, 0x80000000, 0x00007fff
