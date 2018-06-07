/ control.s (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes

        .globl  __control87
        .globl  _control87

        .text


/ unsigned _control87 (unsigned new, unsigned mask)

#define cw_old          0(%esp)
#define cw_new          2(%esp)
/define ret_addr        4(%esp)
#define new             8(%esp)
#define mask            12(%esp)

_control87:
__control87:
        pushl   %ecx                    /* Dummy (for control word) */
        fstcw   cw_old                  /* Store control word into memory */
        movl    mask, %ecx              /* Get mask */
        jecxz   1f                      /* Do not set cw => done */
        movl    new, %edx               /* Get new bits */
        andl    %ecx, %edx              /* Mask new bits by mask bits */
        notl    %ecx
        movl    cw_old, %eax            /* Get old cw */
        andl    %ecx, %eax              /* Apply mask to old cw */
        orl     %edx, %eax              /* Insert new bits */
        movw    %ax, cw_new             /* Put new cw into memory */
        fldcw   cw_new                  /* Load control word */
1:      popl    %eax                    /* Return old cw in lower 16 bits */
        movzwl  %ax, %eax               /* Clear upper 16 bits */
        ret

