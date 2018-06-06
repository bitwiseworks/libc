/* $Id: tst4.c 642 2003-08-19 00:26:01Z bird $ */
/** @file
 *
 * Testcase no.4
 * For checking that certain _Optlink declarations/defines does not
 * infect code after the ';'. (As we've seen in mozilla lately.)
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

/** pre optlink statement declaration. */
int defaultconvention(int i1, int i2, int i3);

/** function pointer type */
typedef int (_Optlink * pfn)(int i1, int i2, int i3);

/** pre optlink statement declaration. */
extern int asmdefaultconvention(int i1, int i2, int i3);

/** post optlink statement defintion. */
int defaultconvention(int i1, int i2, int i3)
{
    if (i1 != 1)
        return 1;
    if (i2 != 2)
        return 2;
    if (i3 != 3)
        return 3;
    return 0;
}


int main ()
{
    int rc;

    rc = defaultconvention(1, 2, 3);
    if (!rc)
        rc = asmdefaultconvention(1, 2, 3);
    if (!rc)
        printf("tst4: Success.\n");
    else
        printf("tst4: failure.\n");
    return rc;
}
