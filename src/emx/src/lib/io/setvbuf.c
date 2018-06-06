/* setvbuf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>

int _STD(setvbuf) (FILE *stream, char *buffer, int mode, size_t size)
{
  if (!(stream->_flags & _IOOPEN)
      || (mode != _IONBF && mode != _IOFBF && mode != _IOLBF))
    return EOF;

  STREAM_LOCK (stream);
  fflush_unlocked (stream);
  if ((stream->_flags & _IOBUFMASK) == _IOBUFLIB)
    free (stream->_buffer);
  stream->_flags &= ~(_IOBUFMASK|_IOLBF|_IOFBF|_IONBF);

  if (mode == _IONBF)
    {
      stream->_buf_size = 1;
      stream->_buffer = &stream->_char_buf;
      stream->_flags |= _IONBF|_IOBUFCHAR;
    }
  else if (buffer != NULL && size >= 1 && size <= 0x400000)
    {
      /* ANSI X3.159-1989, 4.9.5.6: "If buf is not a null pointer, the
         array it points to may be used instead of a buffer allocated
         by the setvbuf function."

         We use that array unless SIZE is invalid. */

      stream->_buf_size = size;
      stream->_buffer = buffer;
      stream->_flags |= mode|_IOBUFUSER;
    }
  else
    {
      /* ANSI X3.159-1989 doesn't assign any meaning to SIZE in this
         case.  We use SIZE as size of the buffer to allocate; if SIZE
         is invalid, use BUFSIZ instead. */

      if (size < 1 || size > 0x400000)
        size = BUFSIZ;
      buffer = malloc (size);
      if (buffer == NULL)
        {
          STREAM_UNLOCK (stream);
          return EOF;
        }
      stream->_buf_size = size;
      stream->_buffer = buffer;
      stream->_flags |= mode|_IOBUFLIB;
    }
  stream->_ptr = stream->_buffer;
  stream->_rcount = 0;
  stream->_wcount = 0;
  STREAM_UNLOCK (stream);
  return 0;
}
