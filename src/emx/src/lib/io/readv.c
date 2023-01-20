/* readv.c (emx+gcc) -- Copyright (c) 1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <io.h>
#include <alloca.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/syslimits.h>

int _STD(readv) (int handle, const struct iovec *iov, int iovcnt)
{
/** @todo make readv a syscall so tcpip can do it's own handling. */
  int i, n;
  size_t total, len;
  char *buf, *mp, *p;

  if (iovcnt <= 0 || iovcnt > IOV_MAX)
    {
      errno = EINVAL;
      return -1;
    }

  total = 0;
  for (i = 0; i < iovcnt; ++i)
    {
      if (iov[i].iov_len < 0)
        {
          errno = EINVAL;
          return -1;
        }
      total += iov[i].iov_len;
    }

  mp = NULL;
  if (total <= 0x1000)
    buf = alloca (total);
  else
    buf = mp = malloc (total);
  if (buf == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  n = read (handle, buf, total);
  if (n > 0)
    {
      total = (size_t)n; n = 0; p = buf;
      for (i = 0; total > 0 && i < iovcnt; ++i)
        {
          len = (size_t)iov[i].iov_len;
          if (len > total) len = total;
          memcpy (iov[i].iov_base, p, len);
          p += len; total -= len; n += len;
        }
    }
  if (mp != NULL)
    free (mp);
  return n;
}
