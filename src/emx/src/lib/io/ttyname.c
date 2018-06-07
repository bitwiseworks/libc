/* ttyname.c (emx+gcc) -- Copyright (c) 1995-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/thread.h>

char *_STD(ttyname) (int handle)
{
  __LIBC_PTHREAD pThrd = __libc_threadCurrent ();

  if (__ttyname (handle, pThrd->szTTYNameBuf, sizeof (pThrd->szTTYNameBuf)) == 0)
    return pThrd->szTTYNameBuf;
  else
    return NULL;
}
