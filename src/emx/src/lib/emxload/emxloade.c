/* emxloade.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <errno.h>
#include <sys/emxload.h>

int _emxload_env (const char *envname)
{
  const char *p, *q;
  long n;

  p = getenv (envname);
  if (p == NULL)
    return -1;
  errno = 0;
  n = strtol (p, (char **)&q, 0);
  if (errno != 0 || q == p || *q != 0 || n < 1)
    return -1;
  _emxload_this ((int)n * 60);
  return 0;
}
