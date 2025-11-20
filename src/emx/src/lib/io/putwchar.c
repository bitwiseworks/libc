/*
    putwchar.c -- copied from putchar.c
    Copyright (c) 2020 bww bitwise works GmbH
*/

#define _GNU_SOURCE /* for _unlocked prototypes */

#include "libc-alias.h"
#include <stdio.h>
#include <wchar.h>

wint_t _STD(putwchar)(wchar_t c)
{
    return putwc(c, stdout);
}

wint_t _STD(putwchar_unlocked)(wchar_t c)
{
    return putwc_unlocked(c, stdout);
}
