/* _str_rd.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <errno.h>
#include <emx/io.h>

int _stream_read (int fd, void *buf, size_t nbyte)
{
  int r;

  do
    {
      r = read (fd, buf, nbyte);
    } while (r == -1 && errno == EINTR);
  return r;
}
