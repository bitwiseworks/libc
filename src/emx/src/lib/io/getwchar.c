/*
    getwchar.c -- copied from getchar.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#define _GNU_SOURCE /* for _unlocked prototypes */

#include "libc-alias.h"
#include <stdio.h>
#include <wchar.h>

wint_t _STD(getwchar)(void)
{
    return getwc(stdin);
}

wint_t _STD(getwchar_unlocked)(void)
{
    return getwc_unlocked(stdin);
}
