/* fgetpos.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

int _STD(fgetpos) (FILE *stream, fpos_t *pos)
{
  __off_t off;
  *pos = EOF;                           /* crash test */
  off = ftello (stream);
  if (off == EOF)
    return EOF;
  *pos = (fpos_t)off;
  return 0;
}
