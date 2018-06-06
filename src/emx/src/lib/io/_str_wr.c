/* _str_wr.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <errno.h>
#include <emx/io.h>

int _stream_write (int fd, const void *buf, size_t nbyte)
{
  int r;

  do
    {
      r = write (fd, buf, nbyte);
    } while (r == -1 && errno == EINTR);
  return r;
}
