/* dup2.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                    -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <io.h>
#include <emx/syscalls.h>

int _STD(dup2)(int handle1, int handle2)
{
  /* TODO: Block signals */
  if (__dup2(handle1, handle2) < 0)
    return -1;
  return handle2;
}
