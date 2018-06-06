/* $Id: weak4-main.c 1244 2004-02-15 07:56:13Z bird $ */
/** @file
 *
 * Testcase for #905 checking that all types of weak symbols
 * is fixed up correctly when a displacement used specified.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2004 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

int weakundef[10]               = {0,1,2,3,4,5,6,7,8,9};
int weakundef_externdefault[10] = {0,1,2,3,4,5,6,7,8,9};

extern int weakbss[10];
extern int weakbss_localdefault_extrn[10];

int main()
{
    int i;
    int rcRet = 0;
    for (i = 0; i < 10; i++)
    {
        weakbss[i] = i;
        weakbss_localdefault_extrn[i] = i + 10;
    }

    if (check_weaktext())
    {
        printf("weak4: weaktext failed\n");
        rcRet++;
    }

    if (check_weakdata())
    {
        printf("weak4: weakdata failed\n");
        rcRet++;
    }

    if (check_weakbss())
    {
        printf("weak4: weakbss failed\n");
        rcRet++;
    }

    if (check_weakundef())
    {
        printf("weak4: weakundef failed\n");
        rcRet++;
    }

    if (check_weakabs())
    {
        printf("weak4: weakabs failed\n");
        rcRet++;
    }

    if (check_weaktext_localdefault())
    {
        printf("weak4: weaktext_localdefault failed\n");
        rcRet++;
    }

    if (check_weakdata_localdefault())
    {
        printf("weak4: weakdata_localdefault failed\n");
        rcRet++;
    }

    if (check_weakbss_localdefault())
    {
        printf("weak4: weakbss_localdefault failed\n");
        rcRet++;
    }

/* doesn't work. AOUT / GAS problem.
    if (check_weakundef_externdefault())
    {
        printf("weak4: weakundef_externdefault failed\n");
        rcRet++;
    }
 */

    if (check_weakabs_localdefault())
    {
        printf("weak4: weakabs_localdefault failed\n");
        rcRet++;
    }

    if (!rcRet)
        printf("weak4: success\n");
    else
        printf("weak4: %d failures\n", rcRet);

    return rcRet;
}
