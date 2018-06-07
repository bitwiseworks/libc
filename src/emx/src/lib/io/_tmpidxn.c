/* _tmpidxn.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */
/*                         Copyright (c) 1991-1993 by Kolja Elsaesser */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <emx/io.h>
#include "_tmp.h"


/* Returns _tmpidx if successful, -1 on error. */

int _tmpidxnam (char *string)
{
  int saved_errno, idx_start, ret;
  size_t len;

  len = strlen (P_tmpdir);
  memcpy (string, P_tmpdir, len);
  if (len > 0 && !_trslash (string, len, 0))
    string[len++] = '/';
  saved_errno = errno;
  TMPIDX_LOCK;
  idx_start = _tmpidx;
  for (;;)
    {
      if (_tmpidx >= IDX_HI)
        _tmpidx = IDX_LO;
      else
        ++_tmpidx;
      if (_tmpidx == idx_start)
        {
          errno = EINVAL;
          ret = -1;
          break;
        }
      _itoa (_tmpidx, string + len, 10);
      strcat (string + len, ".tmp");
      errno = 0;
      if (access (string, 0) != 0)
        {
          if (errno == ENOENT)
            ret = _tmpidx;
          else
            ret = -1;
          break;
        }
    }
  TMPIDX_UNLOCK;
  if (ret != -1)
    errno = saved_errno;
  return ret;
}
