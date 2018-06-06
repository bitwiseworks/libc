/* readkbd.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <emx/syscalls.h>

int _read_kbd (int echo, int wait, int sig)
{
  return __read_kbd (echo, wait, sig);
}
