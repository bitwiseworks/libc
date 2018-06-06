    .file "weak4-2.s"


    .text

    .align 4,0xcc
    nop
    int $41
    nop

    .align 3,0xcc
    .globl _check_weaktext
    /*
     * Check up weak text.
     */
_check_weaktext:
    xorl    %eax, %eax
    inc     %eax                        /* eax = non-zero */
    call    _weaktext+6
    orl     %eax, %eax
    jnz     failure

    inc     %eax                        /* eax = non-zero */
    leal    _weaktext+6, %ecx
    call    *%ecx
    orl     %eax, %eax
    jnz     failure
    jmp     done


    .align 3,0xcc
    .globl _check_weakdata
    /*
     * Check up weak data.
     */
_check_weakdata:
    movl    _weakdata+12, %eax
    cmpl    $3, %eax
    jnz     failure

    lea     _weakdata+20, %eax
    movl    (%eax), %eax
    cmpl    $5, %eax
    jnz     failure
    jmp     done

    .align 3,0xcc
    .globl _check_weakbss
    /*
     * Check up weak bss.
     */
_check_weakbss:
    movl    _weakbss+12, %eax
    cmpl    $3, %eax
    jnz     failure

    lea     _weakbss+20, %eax
    movl    (%eax), %eax
    cmpl    $5, %eax
    jnz     failure
    jmp     done

    .align 3,0xcc
    .globl _check_weakundef
    /*
     * Check up weak undef (in main).
     */
_check_weakundef:
    .weak _weakundef
    movl    _weakundef+12, %eax
    cmpl    $3, %eax
    jnz     failure

    lea     _weakundef+20, %eax
    movl    (%eax), %eax
    cmpl    $5, %eax
    jnz     failure
    jmp     done

    .align 3,0xcc
    .globl _check_weakabs
    /*
     * Check up weak abs (=0xdead)
     */
_check_weakabs:
    leal    _weakabs, %eax
    cmpl    $0xdead, %eax
    jnz     failure

    leal    _weakabs+2, %eax
    cmpl    $0xdeaf, %eax
    jnz     failure
    jmp     done


    .align 3,0xcc
    .globl _check_weaktext_localdefault
    /*
     * Check up weak text with a local default.
     */
_check_weaktext_localdefault:
    /* resolved weak */
    .weak _weaktext_localdefault_extrn
    .set _weaktext_localdefault_extrn, _weaktext_localdefault
    xorl    %eax, %eax
    inc     %eax                        /* eax = non-zero */
    call    _weaktext_localdefault_extrn+5
    orl     %eax, %eax
    jnz     failure

    inc     %eax                        /* eax = non-zero */
    leal    _weaktext_localdefault_extrn+5, %ecx
    call    *%ecx
    orl     %eax, %eax
    jnz     failure

    /* unresolved weak */
    .weak _weaktext_localdefault_extrn_unresolved
    .set _weaktext_localdefault_extrn_unresolved, _weaktext_localdefault
    xorl    %eax, %eax
    inc     %eax                        /* eax = non-zero */
    call    _weaktext_localdefault_extrn_unresolved+6
    orl     %eax, %eax
    jnz     failure

    inc     %eax                        /* eax = non-zero */
    leal    _weaktext_localdefault_extrn_unresolved+6, %ecx
    call    *%ecx
    orl     %eax, %eax
    jnz     failure
    jmp     done


    .align 3,0xcc
    .globl _check_weakdata_localdefault
    /*
     * Check up weak data with a local default.
     */
_check_weakdata_localdefault:
    /* resolved weak */
    .weak _weakdata_localdefault_extrn
    .set _weakdata_localdefault_extrn, _weakdata_localdefault
    movl    _weakdata_localdefault_extrn+12, %eax
    cmpl    $13, %eax
    jnz     failure

    lea     _weakdata_localdefault_extrn+20, %eax
    movl    (%eax), %eax
    cmpl    $15, %eax
    jnz     failure

    /* unresolved weak */
    .weak _weakdata_localdefault_extrn_unresolved
    .set _weakdata_localdefault_extrn_unresolved, _weakdata_localdefault
    movl    _weakdata_localdefault_extrn_unresolved+12, %eax
    cmpl    $3, %eax
    jnz     failure

    lea     _weakdata_localdefault_extrn_unresolved+20, %eax
    movl    (%eax), %eax
    cmpl    $5, %eax
    jnz     failure
    jmp     done


    .align 3,0xcc
    .globl _check_weakbss_localdefault
    /*
     * Check up weak bss with a local default.
     */
_check_weakbss_localdefault:
    /* resolved weak */
    .weak _weakbss_localdefault_extrn
    .set _weakbss_localdefault_extrn, _weakbss_localdefault
    movl    _weakbss_localdefault_extrn+12, %eax
    cmpl    $13, %eax
    jnz     failure

    lea     _weakbss_localdefault_extrn+20, %eax
    movl    (%eax), %eax
    cmpl    $15, %eax
    jnz     failure

    /* unresolved weak */
    .weak _weakbss_localdefault_extrn_unresolved
    .set _weakbss_localdefault_extrn_unresolved, _weakbss_localdefault
    movl    _weakbss_localdefault_extrn_unresolved+12, %eax
    cmpl    $3, %eax
    jnz     failure

    lea     _weakbss_localdefault_extrn_unresolved+20, %eax
    movl    (%eax), %eax
    cmpl    $5, %eax
    jnz     failure
    jmp     done


    .align 3,0xcc
    .globl _check_weakundef_externdefault
    /*
     * Check up weak bss with an undefined default.
     */
_check_weakundef_externdefault:
    /* resolved weak */
    .weak _weakundef_externdefault_extrn
    .set _weakundef_externdefault_extrn, _weakundef_externdefault
    movl    _weakundef_externdefault_extrn+8, %eax
    cmpl    $12, %eax
    jnz     failure

    lea     _weakundef_externdefault_extrn+20, %eax
    movl    (%eax), %eax
    cmpl    $15, %eax
    jnz     failure

    /* unresolved weak */
    .weak _weakundef_externdefault_extrn_unresolved
    .set _weakundef_externdefault_extrn_unresolved, _weakundef_externdefault
    movl    _weakundef_externdefault_extrn_unresolved+12, %eax
    cmpl    $3, %eax
    jnz     failure

    lea     _weakundef_externdefault_extrn_unresolved+20, %eax
    movl    (%eax), %eax
    cmpl    $5, %eax
    jnz     failure
    jmp     done


    .align 3,0xcc
    .globl _check_weakabs_localdefault
    /*
     * Check up weak bss with an undefined default.
     */
_check_weakabs_localdefault:
    /* resolved weak */
    .weak _weakabs_localdefault_extrn
    .set _weakabs_localdefault_extrn, _weakabs_localdefault
    leal    _weakabs_localdefault_extrn, %eax
    cmpl    $0xbeef, %eax
    jnz     failure

    leal    _weakabs_localdefault_extrn+2, %eax
    cmpl    $0xbef1, %eax
    jnz     failure

    /* unresolved weak */
    .weak _weakabs_localdefault_extrn_unresolved
    .set _weakabs_localdefault_extrn_unresolved, _weakabs_localdefault
    leal    _weakabs_localdefault_extrn_unresolved, %eax
    cmpl    $0xdead, %eax
    jnz     failure

    leal    _weakabs_localdefault_extrn_unresolved+2, %eax
    cmpl    $0xdeaf, %eax
    jnz     failure
    jmp     done


done:
    xorl    %eax, %eax
    ret
failure:
    movl    $1, %eax
    ret


/*
 * Weak text.
 */
    .align 4,0xcc
    nop
    int $42
    nop
    .align 3,0xcc
    .weak _weaktext
_weaktext:
_weaktext_localdefault:
    ret                                 /* 0 */
    ret                                 /* 1 */
    ret                                 /* 2 */
    ret                                 /* 3 */
    ret                                 /* 4 */
    ret                                 /* 5 */
    xorl %eax, %eax                     /* 6 */
    ret
    ret
    ret
    ret


/*
 * Weak data.
 */
.data
    .space 16
    .weak _weakdata
_weakdata:
_weakdata_localdefault:
    .long   0
    .long   1
    .long   2
    .long   3
    .long   4
    .long   5
    .long   6
    .long   7


/*
 * Weak bss (initiated by main()).
 */
.bss
    .space 32
    .weak _weakbss
_weakbss:
_weakbss_localdefault:
    .space 40


/*
 * Weak abs
 */
    .weak _weakabs
    .equ _weakabs, 0xdead
    .global _weakabs_localdefault
    .equ _weakabs_localdefault, 0xdead
