/* _fill.c (emx+gcc) -- Copyright (c) 1990-2000 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <emx/io.h>

/* Note: _fill() now (as of emx 0.9a fix04) assumes that `rcount' has
   been decremented prior to calling _fill().  See ungetc.c for
   details. */

int _fill (FILE *stream)
{
  /* Undo decrementing of `rcount'.  This wasn't required in earlier
     versions as `rcount' was set but not examined by _fill().
     Moreover, this fixes the bug of `rcount' being wrong if _fill()
     fails. */

  ++stream->_rcount;

  /* Fail (without setting errno) for sscanf(). */

  if ((stream->_flags & _IOSPECIAL) && stream->_flush == NULL)
    return EOF;

  /* Fail if the stream is not open. */

  if (!(stream->_flags & _IOOPEN))
    {
      errno = EACCES;
      return EOF;
    }

  /* switch to read mode if necessary. */

  if (stream->_flags & _IOWRT)
    {
      if (stream->_flags & _IORW)
        {
          int rc = _fseek_unlocked (stream, 0, SEEK_CUR);
          if (rc)
            return EOF;
        }
      if (stream->_flags & _IOWRT)
        {
          stream->_flags |= _IOERR;
          errno = EACCES;
          return EOF;
        }
    }

  if (stream->_ungetc_count != 0)
    {
      int c;

      /* There are pushed-back characters.  Return the most recent one
         and undo the effects of one ungetc() call. */

      stream->_ungetc_count -= 1;
      c = (unsigned char)stream->_buffer[stream->_ungetc_count];

      if (stream->_ungetc_count == 0)
        stream->_rcount = -stream->_rcount;

      return c;
    }

  if (stream->_flags & _IOSPECIAL)
    {
      if (stream->_flush != NULL)
        return stream->_flush (stream, _FLUSH_FILL);
      errno = EACCES;
      return EOF;
    }

  /* Switch to read mode. */

  stream->_flags |= _IOREAD;
  stream->_wcount = 0;
  if (nbuf (stream))
    _fbuf (stream);
  else
    stream->_ptr = stream->_buffer;

  /* Fill the buffer. */

  stream->_flags &= ~_IOUNGETC;
  stream->_rcount = _stream_read (stream->_handle, stream->_buffer,
                                  stream->_buf_size);
  if (stream->_rcount < 0)
    {
      stream->_flags |= _IOERR;
      stream->_rcount = 0;
      return EOF;
    }

  /* Check for EOF. */

  if (stream->_rcount == 0)
    {
      stream->_flags |= _IOEOF;
      if (stream->_flags & _IORW)
        stream->_flags &= ~(_IOREAD|_IOWRT);
      return EOF;
    }

  /* Return the next character. */

  --stream->_rcount;
  return (unsigned char)*stream->_ptr++;
}
