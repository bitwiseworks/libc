/* fopen.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <share.h>
#include <emx/io.h>

FILE *_STD(fopen) (const char *fname, const char *mode)
{
  FILE *f;

  f = _newstream ();
  if (f != NULL)
    f = _openstream (f, fname, mode, SH_DENYNO, 1);
  return f;
}
