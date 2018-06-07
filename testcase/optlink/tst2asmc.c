/* $Id: tst2asmc.c 678 2003-09-09 19:21:51Z bird $
 *
 * Optlink testcase no. 2.
 * C source of the ASM file.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

struct sss
{
    int a;
    int b;
};

int _Optlink asmfoo(int i1, struct sss s1, void *pv, float rf1, struct sss s2)
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

