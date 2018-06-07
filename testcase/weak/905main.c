/* $Id: 905main.c 1237 2004-02-15 01:24:40Z bird $ */
/** @file
 *
 * Testcase for but #905.
 * The problem is the fixups generated for weak symbols, in particular
 * the weak virtual function tables (i.e. an data array).
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2004 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
int giFoo1 = 1;                           /* make sure we skip into the data segment. */

/* weak arrays */
#pragma weak gaiWeak1
int gaiWeak1[10] = {0,1,2,3,4,5,6,7,8,9};
#pragma weak gaiWeak2
int gaiWeak2[10] = {0,1,2,3,4,5,6,7,8,9};
/* controll array. */
int gaiNotWeak[10] = {0,1,2,3,4,5,6,7,8,9};

int giFoo2 = 1;                           /* make sure we skip into the data segment. */



int main()
{
    int i;
    int *pai;
    int rcRet = 0;

    i = gaiWeak1[2];
    if (i != 2)
    {
        printf("gaiWeak1[2] == %d, expected 2.\n", i);
        rcRet++;
    }
    i = gaiWeak2[2];
    if (i != 2)
    {
        printf("gaiWeak2[2] == %d, expected 2.\n", i);
        rcRet++;
    }
    i = gaiNotWeak[2];
    if (i != 2)
    {
        printf("gaiNotWeak[2] == %d, expected 2.\n", i);
        rcRet++;
    }

    pai = &gaiWeak1[5];
    i = *pai;
    if (i != 5)
    {
        printf("gaiWeak1[5] == %d, expected 5.\n", i);
        rcRet++;
    }
    pai = &gaiWeak2[5];
    i = *pai;
    if (i != 5)
    {
        printf("gaiWeak2[5] == %d, expected 5.\n", i);
        rcRet++;
    }
    pai = &gaiNotWeak[5];
    i = *pai;
    if (i != 5)
    {
        printf("gaiNotWeak[5] == %d, expected 5.\n", i);
        rcRet++;
    }

    return rcRet;
}
