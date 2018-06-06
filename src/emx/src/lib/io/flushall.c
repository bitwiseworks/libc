/* flushall.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>

int _STD(flushall) (void)
{
  int i, n;
  struct streamvec *sv;

  /* Ignore locked streams to avoid deadlock on process
     termination. */

  n = 0;
  for (sv = _streamvec_head; sv != NULL; sv = sv->pNext)
    for (i = 0; i < sv->cFiles; ++i)
      if ((sv->aFiles[i]._flags & _IOOPEN) && STREAM_LOCK_NOWAIT (&sv->aFiles[i]))
        {
          ++n;
          fflush_unlocked (&sv->aFiles[i]);
          STREAM_UNLOCK (&sv->aFiles[i]);
        }
  return n;
}
