/* _fsopen.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <emx/io.h>

FILE *_fsopen (const char *fname, const char *mode, int shflag)
{
  FILE *f;

  f = _newstream ();
  if (f != NULL)
    f = _openstream (f, fname, mode, shflag, 1);
  return f;
}
