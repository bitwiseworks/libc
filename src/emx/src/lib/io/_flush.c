/* _flush.c (emx+gcc) -- Copyright (c) 1990-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <emx/io.h>

int _flush (int c, FILE *stream)
{
  stream->_wcount = 0;          /* Undo decrement to avoid negative value */

  /* Don't flag an error attempting to write beyond the end of the
     string for snprintf() as we need the total number of characters
     printed. */

  if ((stream->_flags & _IOSPECIAL) && stream->_flush == NULL)
    return (unsigned char)c;

  /* Fail if the stream is not open or there is no function for
     flushing the buffer. */

  if (!(stream->_flags & _IOOPEN) || stream->_flush == NULL)
    {
      errno = EACCES;
      return EOF;
    }

  /* switch to write mode if necessary. */

  if (stream->_flags & _IOREAD)
    {
      if (stream->_flags & _IORW)
        {
          int rc = _fseek_unlocked (stream, 0, SEEK_CUR);
          if (rc)
            return EOF;
        }
      if (stream->_flags & _IOREAD)
        {
          stream->_flags |= _IOERR;
          errno = EACCES;
          return EOF;
        }
    }

  /* Flush the buffer and put C into the buffer. */

  if (stream->_flush (stream, (unsigned char)c) != 0)
    return EOF;
  return (unsigned char)c;
}
