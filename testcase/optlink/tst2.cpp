/* $Id: tst2.cpp 678 2003-09-09 19:21:51Z bird $ */
/** @file
 *
 * Testcase no.2
 * Testing the mixing of args for regs and args which doesn't go into regs.
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
#ifdef __cplusplus
extern "C"
#else
extern
#endif
int _Optlink asmfoo(int i1, struct sss s1, void *pv, float rf1, struct sss s2);

struct sss
{
    int a;
    int b;
};


int _Optlink foo(int i1, struct sss s1, void *pv, float rf1, struct sss s2)
{
    if (i1 != 1)
        return 1;
    if (s1.a != 2)
        return 2;
    if (s1.b != 3)
        return 3;
    if (pv != (void*)4)
        return 4;
    if (rf1 != (float)1.5)
        return 5;
    if (s2.a != 6)
        return 6;
    if (s2.b != 7)
        return 7;
    return 0;
}

int main()
{
    int         rcRet = 0;
    int         rc;
    struct sss  s1 = {2, 3};
    struct sss  s2 = {6, 7};

    /* calling vac generated code */
    rc = asmfoo(1, s1, (void*)4, 1.5, s2);
    switch (rc)
    {
        case 0: printf("tst2: asmfoo: success.\n"); break;
        case 1: printf("tst2: asmfoo: i1 check failed.\n"); break;
        case 2: printf("tst2: asmfoo: s1.a check failed.\n"); break;
        case 3: printf("tst2: asmfoo: s1.b check failed.\n"); break;
        case 4: printf("tst2: asmfoo: pv check failed.\n"); break;
        case 5: printf("tst2: asmfoo: rf1 check failed.\n"); break;
        case 6: printf("tst2: asmfoo: s2.a check failed.\n"); break;
        case 7: printf("tst2: asmfoo: s2.b check failed.\n"); break;
        default:
            printf("tst2: asmfoo: failed test %d - internal error\n", rc);
            break;
    }
    if (rc && !rcRet)
        rcRet = rc;

    /* all gcc */
    rc = foo(1, s1, (void*)4, 1.5, s2);
    switch (rc)
    {
        case 0: printf("tst2: foo: success.\n"); break;
        case 1: printf("tst2: foo: asmfoo: i1 check failed.\n"); break;
        case 2: printf("tst2: foo: asmfoo: s1.a check failed.\n"); break;
        case 3: printf("tst2: foo: asmfoo: s1.b check failed.\n"); break;
        case 4: printf("tst2: foo: asmfoo: pv check failed.\n"); break;
        case 5: printf("tst2: foo: asmfoo: rf1 check failed.\n"); break;
        case 6: printf("tst2: foo: asmfoo: s2.a check failed.\n"); break;
        case 7: printf("tst2: foo: asmfoo: s2.b check failed.\n"); break;
        default:
            printf("tst2: foo: failed test %d - internal error\n", rc);
            break;
    }
    if (rc && !rcRet)
        rcRet = rc;

    return rcRet;
}
