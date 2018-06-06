/* perror.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>

void _STD(perror) (const char *string)
{
  int e = errno;

  if (string != NULL && *string != 0)
    {
      fputs (string, stderr);
      fputs (": ", stderr);
    }
  if (e >= 0 && e < sys_nerr)
    fputs (sys_errlist[e], stderr);
  else
    fprintf (stderr, "Unknown error %d", e);
  fputc ('\n', stderr);
}
