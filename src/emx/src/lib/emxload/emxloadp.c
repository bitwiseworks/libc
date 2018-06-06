/* emxloadp.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/emxload.h>
#include <emx/emxload.h>

int _emxload_prog (const char *name, int seconds)
{
  char buf2[MAXPATHLEN];

  if (seconds != _EMXLOAD_INDEFINITE && seconds < 0)
    return -1;
  if (   _path2 (name, ".exe", buf2, sizeof (buf2)) != 0
      || _abspath (buf2, buf2, sizeof (buf2)) != 0)
    return -1;
  return _emxload_request (_EMXLOAD_LOAD, buf2, seconds);
}
