/* _fsetmod.c (emx+gcc) -- Copyright (c) 1990-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>

int _fsetmode (FILE *stream, const char *mode)
{
  int i;

  if (*mode == 'b')
    i = O_BINARY;
  else if (*mode == 't')
    i = O_TEXT;
  else
    {
      errno = EINVAL;
      return -1;
    }
  if (setmode (stream->_handle, i) == -1)
    return -1;
  return 0;
}
