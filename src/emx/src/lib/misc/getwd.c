/* getwd.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/backend.h>

char *_STD(getwd) (char *buffer)
{
  if (!buffer)
    {
      errno = EINVAL;
      return NULL;
    }
  int rc = __libc_Back_fsDirCurrentGet(buffer, MAXPATHLEN, 0, 0);
  if (rc)
    {
      _strncpy (buffer, strerror (-rc), MAXPATHLEN);
      errno = -rc;
      return NULL;
    }
  return buffer;
}
