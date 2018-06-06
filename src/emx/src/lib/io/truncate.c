/* truncate.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <fcntl.h>

int _STD(truncate) (const char *name, off_t length)
{
  int handle, result;

  handle = open (name, O_RDWR);
  if (handle == -1)
    return -1;
  result = ftruncate (handle, length);
  close (handle);
  return result;
}
