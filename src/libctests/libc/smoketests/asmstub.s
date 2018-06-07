    .text
__text:
    xorl %eax, %eax
    ret

    .data
___data_start:
__data:
    .long   0xba0bab
    .long   __os2dll
    .stabs  "__os2dll", 21, 0, 0, 0xffffffff
    .bss
___bss_start:

