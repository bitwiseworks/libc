/* earemove.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <sys/ea.h>

int _ea_remove (const char *path, int handle, const char *name)
{
  struct _ea ea;

  ea.flags = 0;
  ea.size = 0;
  ea.value = NULL;
  return _ea_put (&ea, path, handle, name);
}
