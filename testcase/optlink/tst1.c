/* $Id: tst1.c 678 2003-09-09 19:21:51Z bird $ */
/** @file
 *
 * Optlink testcase no. 1.
 * Passing stuff in all registers.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

 #include <stdio.h>

/** in asm */
extern int _Optlink asmfoo (int i1, int i2, int i3, double rf1, double rf2, double rf3, double rf4);

extern int _Optlink foo (int i1, int i2, int i3, double rf1, double rf2, double rf3, double rf4)
{
    if (i1 != 1)
        return 1;
    if (i2 != 2)
        return 2;
    if (i3 != 3)
        return 3;
    if (rf1 != 1.1)
        return 4;
    if (rf2 != 1.2)
        return 5;
    if (rf3 != 1.3)
        return 6;
    if (rf4 != 1.4)
        return 7;
    return 0;
}

int main()
{
    int i;
    int rc;
    int rcRet = 0;

    /* calling vac generated code */
    rc = asmfoo(1, 2, 3, 1.1, 1.2, 1.3, 1.4);
    switch (rc)
    {
        case 0: printf("tst1: asmfoo: success.\n"); break;
        case 1: printf("tst1: asmfoo: i1 check failed.\n"); break;
        case 2: printf("tst1: asmfoo: i2 check failed.\n"); break;
        case 3: printf("tst1: asmfoo: i3 check failed.\n"); break;
        case 4: printf("tst1: asmfoo: rf1 check failed.\n"); break;
        case 5: printf("tst1: asmfoo: rf2 check failed.\n"); break;
        case 6: printf("tst1: asmfoo: rf3 check failed.\n"); break;
        case 7: printf("tst1: asmfoo: rf4 check failed.\n"); break;
        default:
            printf("tst1: asmfoo: failed test %d - internal error\n", rc);
            break;
    }
    if (rc && !rcRet)
        rcRet = rc;

    /* all gcc */
    rc = foo(1, 2, 3, 1.1, 1.2, 1.3, 1.4);
    switch (rc)
    {
        case 0: printf("tst1: foo: success.\n"); break;
        case 1: printf("tst1: foo: i1 check failed.\n"); break;
        case 2: printf("tst1: foo: i2 check failed.\n"); break;
        case 3: printf("tst1: foo: i3 check failed.\n"); break;
        case 4: printf("tst1: foo: rf1 check failed.\n"); break;
        case 5: printf("tst1: foo: rf2 check failed.\n"); break;
        case 6: printf("tst1: foo: rf3 check failed.\n"); break;
        case 7: printf("tst1: foo: rf4 check failed.\n"); break;
        default:
            printf("tst1: foo: failed test %d - internal error\n", rc);
            break;
    }
    if (rc && !rcRet)
        rcRet = rc;

    return rcRet;
}
