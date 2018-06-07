/* strerror.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <InnoTekLIBC/thread.h>
#include <sys/errno.h>

char *_STD(strerror) (int errnum)
{
  if (errnum >= 0 && errnum < sys_nerr)
    return (char *)sys_errlist [errnum];
  else
    {
      static char msg[] = "Unknown error ";
      __LIBC_PTHREAD pThrd = __libc_threadCurrent();
      memcpy (pThrd->szStrErrorBuf, msg, sizeof (msg) - 1);
      _itoa (errnum, pThrd->szStrErrorBuf + sizeof (msg) - 1, 10);
      errno = EINVAL; /* this is actually an invalid error number. */
      return pThrd->szStrErrorBuf;
    }
}
