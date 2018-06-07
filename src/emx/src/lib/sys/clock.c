/* sys/clock.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <os2emx.h>
#include <time.h>
#include <emx/syscalls.h>
#include "syscalls.h"

/* This function no longer tries to use QSV_TIME_LOW and QSV_TIME_HIGH
   to increase the time before the return value wraps around.  Now, it
   will wrap around after 49 days. */

clock_t _STD(clock)(void)
{
    ULONG val_ms;
    clock_t result;

    _sys_get_clock(&val_ms);
    result = (val_ms - _sys_clock0_ms) / 10;
    return result;
}
