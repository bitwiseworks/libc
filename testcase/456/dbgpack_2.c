/* $Id: dbgpack_2.c 765 2003-09-30 18:51:48Z bird $ */
/** @file
 *
 * See comment in dbgpack.c.
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


extern const char * bar(void);
int gfFlag = 0;

const char * foo(void)
{
    return bar();
}

