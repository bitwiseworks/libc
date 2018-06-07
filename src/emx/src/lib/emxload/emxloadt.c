/* emxloadt.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <sys/param.h>
#include <sys/emxload.h>

int _emxload_this (int seconds)
{
  char name[MAXPATHLEN];

  if (_execname (name, sizeof (name)) != 0)
    return -1;
  return _emxload_prog (name, seconds);
}
