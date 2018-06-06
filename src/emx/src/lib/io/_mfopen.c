/* _mfopen.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <errno.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <emx/io.h>

static int _mflush (FILE *stream, int c)
{
  int n;

  if (c < 0)
    return 0;
  n = stream->_ptr - stream->_buffer;
  if (n >= stream->_buf_size)
    {
      if (stream->_tmpidx == 0)
        {
          errno = ENOSPC;
          return EOF;
        }
      if (stream->_tmpidx == -2)
        {
          if (stream->_buf_size == 0)
            stream->_buf_size = 512;
          else
            stream->_buf_size *= 2;
        }
      else
        stream->_buf_size += stream->_tmpidx;
      stream->_buffer = realloc (stream->_buffer, stream->_buf_size);
      if (stream->_buffer == NULL)
        {
          stream->_buf_size = 0;
          stream->_wcount = 0;
          errno = ENOMEM;
          return EOF;
        }
    }
  stream->_ptr = stream->_buffer + n;
  *stream->_ptr++ = (char)c;
  ++n;
  stream->_wcount = stream->_buf_size - n;
  return 0;
}



FILE *_mfopen (char *buf, const char *mode, size_t size, int inc)
{
  FILE *stream;
  char *abuf = NULL;

  if (mode[0] != 'w' || mode[1] != 'b' || mode[2] != 0)
    {
      errno = EINVAL;
      return NULL;
    }
  if ((buf == NULL && inc == 0) || (buf != NULL && (size == 0 || inc != 0)))
    {
      errno = EINVAL;
      return NULL;
    }
  stream = _newstream ();
  if (stream == NULL)
    return NULL;
  if (buf == NULL && size != 0)
    {
      buf = abuf = malloc (size);
      if (buf == NULL)
        {
          errno = ENOMEM;
          STREAMV_LOCK;
          _closestream (stream);
          STREAMV_UNLOCK;
          return NULL;
        }
    }
  stream->_buffer = buf;
  stream->_ptr = buf;
  stream->_rcount = 0;
  stream->_wcount = size;
  stream->_handle = -1;
  stream->_flags = _IOOPEN|_IOSPECIAL|_IOBUFUSER|_IOWRT;
  stream->_buf_size = size;
  stream->_flush = _mflush;
  stream->_tmpidx = inc;
  stream->_ungetc_count = 0;
  if (_fmutex_create2 (&stream->__u.__fsem, 0, "LIBC stream mfopen") != 0)
    {
      if (abuf != NULL) free (abuf);
      return NULL;
    }
  return stream;
}


char *_mfclose (FILE *stream)
{
  char *buf;

  if ((stream->_flags & (_IOOPEN|_IOSPECIAL)) != (_IOOPEN|_IOSPECIAL)
      || stream->_flush != _mflush)
    return NULL;

  /* Avoid a time window in which another thread allocates the stream
     and sets _buffer! */

  buf = stream->_buffer;
  STREAMV_LOCK;
  _closestream (stream);
  STREAMV_UNLOCK;
  return buf;
}
