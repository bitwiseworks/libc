/ $Id: appinit.s 810 2003-10-06 00:55:10Z bird $

	.globl  ___init_app
	.globl  __sys_init_ret

	.data

L_ret:	.long   0

	.text

___init_app:
	popl    L_ret
	call    ___init
/ Theoretically we never should get here. Force a SIGSEGV.
	hlt

__sys_init_ret:
	movl    4(%esp), %esp
	jmp     *L_ret
