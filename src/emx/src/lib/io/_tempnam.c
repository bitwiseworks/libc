/* _tempnam.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */
/*                         Copyright (c) 1991-1993 by Kolja Elsaesser */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef __USE_GNU /* __strnlen */
# define __USE_GNU
#endif
#include <string.h>
#include <io.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <emx/io.h>
#include "_tmp.h"


/* Create absolute path name from src, copy it to dst and return 1 if
   it's a directory. Note that this works also with a trailing backslash! */

static int _isdir (char *dst, const char *src)
{
  struct stat s;

  if (_fullpath (dst, src, MAXPATHLEN) != 0)
    return 0;
  return lstat (dst, &s) == 0
      && S_ISDIR(s.st_mode);
}


char *_STD(tempnam) (const char *dir, const char *prefix)
{
  const char *tmpdir;
  char *tmpname, *p, buf[MAXPATHLEN];
  size_t len;
  int saved_errno, idx_start;

  saved_errno = errno;
  tmpdir = NULL;
  p = getenv ("TMP");
  if (tmpdir == NULL && p != NULL && _isdir (buf, p))
    tmpdir = buf;
  if (tmpdir == NULL && dir != NULL && _isdir (buf, dir))
    tmpdir = buf;
  if (tmpdir == NULL)
    tmpdir = P_tmpdir;
  tmpname = malloc (strlen (tmpdir) + 1 + L_tmpnam);
  if (tmpname == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }

  len = strlen (tmpdir);
  memcpy (tmpname, tmpdir, len);
  if (len > 0 && !_trslash (tmpname, len, 0))
    tmpname[len++] = '/';
  p = tmpname + len;

  if (!prefix)
    prefix = "kLC";

  TMPIDX_LOCK;
  idx_start = _tmpidx;
  for (;;)
    {
      if (_tmpidx == IDX_HI)
        _tmpidx = IDX_LO;
      else
        ++_tmpidx;
      if (_tmpidx == idx_start)
        {
          TMPIDX_UNLOCK;
          free (tmpname);
          errno = EINVAL;
          return NULL;
        }
      _itoa (_tmpidx, p, 10);
      strcat (p, ".tmp");
      memmove (p, prefix, __strnlen (prefix, 5));
      errno = 0;
      if (access (tmpname, 0) != 0)
        {
          if (errno == ENOENT)
            break;
          TMPIDX_UNLOCK;
          free (tmpname);
          return NULL;
        }
    }
  TMPIDX_UNLOCK;
  errno = saved_errno;
  if (_fullpath (buf, tmpname, sizeof (buf)) != 0)
    {
      free (tmpname);
      errno = ENOENT;
      return NULL;
    }
  free (tmpname);
  p = strdup (buf);
  if (p != NULL)
    errno = saved_errno;
  else
    errno = ENOMEM;
  return p;
}
