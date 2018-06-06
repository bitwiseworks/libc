/* sys/getppid.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <os2emx.h>
#include "syscalls.h"

int _STD(getppid) (void)
{
  return _sys_ppid;
}
