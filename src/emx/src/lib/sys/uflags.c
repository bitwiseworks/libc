/* sys/uflags.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/uflags.h>
#include "syscalls.h"

int _uflags (int mask, int new_flags)
{
  int ret;

  ret = _sys_uflags;
  _sys_uflags = (_sys_uflags & ~mask) | (new_flags & mask);
  return ret;
}
