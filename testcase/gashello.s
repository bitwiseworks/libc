    .file "gashello.s"

.text
    .global _main
_main:
    push    $string
    /*call    _puts */
    call    __std_puts
    add     $4, %esp
    xorl    %eax, %eax
    ret


.data
	.long	0xba0bab	/* Magic number (error detection) */
	.long   __os2dll	/* list of OS/2 DLL references */

	.stabs  "__os2dll", 21, 0, 0, 0xffffffff

string:
    .asciz    "hello gas world"


