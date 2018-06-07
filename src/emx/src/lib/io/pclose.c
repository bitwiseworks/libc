/* pclose.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <process.h>
#include <errno.h>
#include <emx/io.h>

int _STD(pclose) (FILE *stream)
{
  int rc, write_mode;

  if (!(stream->_flags & _IOOPEN))
    {
      errno = EBADF;
      return -1;
    }
  write_mode = (stream->_flags & _IOWRT);
  if (write_mode && fclose (stream) != 0)
    return -1;
  if (waitpid (stream->_pid, &rc, 0) == -1)
    return -1;
  if (!write_mode && fclose (stream) != 0 && errno != EBADF)
    return -1;
  return rc;
}
