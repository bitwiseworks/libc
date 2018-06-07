/* rewind.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>

void _STD(rewind) (FILE *stream)
{
  STREAM_LOCK (stream);
  fflush_unlocked (stream);
  _fseek_unlocked (stream, 0, SEEK_SET);
  stream->_flags &= ~_IOERR;
  STREAM_UNLOCK (stream);
}
