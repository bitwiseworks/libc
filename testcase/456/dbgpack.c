/* $Id: dbgpack.c 765 2003-09-30 18:51:48Z bird $ */
/** @file
 *
 * Testcase for /DBGPACK.
 * We're just forcing a bunch of HLL types in dbgpack.c most of which will
 * not occure in dbgpack_2.c. rdrdump will show the same typetable for
 * both dbgpack.c and dbgpack_2.c if the packing works. (I.e. it dumps the
 * global type table.) If it doesn't work dbgpack.c will have a lot more
 * types in it's table thank dbgpack_2.c.
 *
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */
#define INCL_BASE
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


extern const char * foo(void);
extern int gfFlag;

const char * bar(void)
{
    /* This isn't supposed to make sense btw. ;) */
    PFILEFINDBUF3  pfst3 = NULL;
    if (gfFlag && DosOpen && DosClose && !pfst3)
        return "set";
    return "clear";
}

int main()
{
    if (strcmp(foo(), "set"))
        return 0;
    return 1;
}

