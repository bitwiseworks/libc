	.file	"905-2.s"

	.data
	.align 2
	.long	42
	.long	42
	.long	42
	.long	42

	.weak	_gaiWeak
	.align 5
_gaiWeak:
	.long	0
	.long	1
	.long	2
	.long	3
	.long	4
	.weak	_gaiWeak2
	.align 5

.globl _gaiNotWeak
	.align 5
_gaiNotWeak:
	.long	0
	.long	1
	.long	2
	.long	3
	.long	4



	.text
	.align 2
_foo:

    leal	_gaiWeak, %eax
	leal	_gaiNotWeakExt, %eax
    leal	_gaiWeak, %eax
	leal	_gaiNotWeak, %eax

    movl	_gaiWeak, %eax
	movl	_gaiNotWeakExt, %eax
    movl	_gaiWeak, %eax
	movl	_gaiNotWeak, %eax

    leal	_gaiWeak+8, %eax
	leal	_gaiNotWeakExt+8, %eax
    leal	_gaiWeak+8, %eax
	leal	_gaiNotWeak+8, %eax

    movl	_gaiWeak+8, %eax
	movl	_gaiNotWeakExt+8, %eax
    movl	_gaiWeak+8, %eax
	movl	_gaiNotWeak+8, %eax

	leal	_gaiWeak+14, %eax
	leal	_gaiNotWeakExt+14, %eax
	leal	_gaiWeak+14, %eax
	leal	_gaiNotWeak+14, %eax

	movl	_gaiWeak+14, %eax
	movl	_gaiNotWeakExt+14, %eax
	movl	_gaiWeak+14, %eax
	movl	_gaiNotWeak+14, %eax
	ret

