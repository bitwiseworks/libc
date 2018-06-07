/* fclose.c (emx+gcc) -- Copyright (c) 1990-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <386/builtin.h>
#include <sys/fmutex.h>
#include <emx/io.h>
#include <stdio.h>

int _STD(fclose) (FILE *stream)
{
  int result;
  char buf[L_tmpnam];

  result = EOF;
  if ((stream->_flags & _IOOPEN) && !(stream->_flags & _IOSPECIAL))
    {
      result = 0;
      result = fflush (stream);
      if (close (stream->_handle) < 0)
        result = EOF;
      if (result == 0 && (stream->_flags & _IOTMP))
        {
          _itoa (stream->_tmpidx, buf, 10);
          strcat (buf, ".tmp");
          if (remove (buf) != 0)
            result = EOF;
        }
      if ((stream->_flags & _IOBUFMASK) == _IOBUFLIB)
        free (stream->_buffer);
    }
  STREAMV_LOCK;
  _closestream (stream);
  STREAMV_UNLOCK;
  return result;
}
