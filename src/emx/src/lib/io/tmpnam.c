/* tmpnam.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */
/*                       Copyright (c) 1991-1993 by Kolja Elsaesser */

#include "libc-alias.h"
#include <stdio.h>
#include <InnoTekLIBC/thread.h>
#include "_tmp.h"


char *_STD(tmpnam) (char *string)
{
  if (string == NULL)
    {
      __LIBC_PTHREAD pThrd = __libc_threadCurrent ();
      string = pThrd->szTmpNamBuf;
    }
  if (_tmpidxnam (string) >= 0)
    return string;
  else
    return NULL;
}
