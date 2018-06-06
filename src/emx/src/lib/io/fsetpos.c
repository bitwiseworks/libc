/* fsetpos.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

int _STD(fsetpos) (FILE *stream, const fpos_t *pos)
{
  if (fseeko (stream, (__off_t)*pos, SEEK_SET) != 0)
    return -1;
  return 0;
}
