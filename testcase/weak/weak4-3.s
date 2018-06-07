    .file "weak4-3.s"


.text

/*
 * Weak text.
 */
    .align 4,0xcc
    nop
    int $42
    nop
    .align 3,0xcc
    .global _weaktext_localdefault_extrn
_weaktext_localdefault_extrn:
    ret                                 /* 0 */
    ret                                 /* 1 */
    ret                                 /* 2 */
    ret                                 /* 3 */
    ret                                 /* 4 */
    xorl %eax, %eax                     /* 5 */
    ret
    ret
    ret
    ret
    ret


.data

/*
 * Weak data
 */
    .align 3,0xcc
    .global _weakdata_localdefault_extrn
_weakdata_localdefault_extrn:
    .global _weakundef_externdefault_extrn
_weakundef_externdefault_extrn:
    .long   10
    .long   11
    .long   12
    .long   13
    .long   14
    .long   15
    .long   16
    .long   17
    .long   18
    .long   19


.bss

/*
 * Weak bss
 */
    .align 3,0xcc
    .global _weakbss_localdefault_extrn
_weakbss_localdefault_extrn:
    .space 40


/*
 * Weak abs
 */
    .global _weakabs_localdefault_extrn
    .equ _weakabs_localdefault_extrn, 0xbeef

