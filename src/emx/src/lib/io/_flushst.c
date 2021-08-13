/* _flushst.c (emx+gcc) -- Copyright (c) 1990-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <emx/io.h>

int _flushstream (FILE *stream, int c)
{
  int n, w, fh;
  char ch;

  fh = stream->_handle;
  if (c == _FLUSH_FILL)
    {
      /* Does not happen. */

      errno = EACCES;
      return EOF;
    }
  else
    {
      stream->_flags |= _IOWRT;       /* Switch to write mode */
      stream->_rcount = 0;
      stream->_flags &= ~_IOEOF;      /* Clear EOF flag, writing! */
      stream->_wcount = 0;            /* Maybe negative at this point */
      if (_nbuf (stream))
        _fbuf (stream);
      if (_bbuf (stream))
        {
          n = stream->_ptr - stream->_buffer;
          if (n > 0)            /* n should never be < 0 */
            {
              /* Try to use a single write() call.  This can succeed
                 only if _flush() is called before the buffer is
                 full.  This can happen only when writing a '\n' to a
                 line-buffered file, which is exactly the case where
                 we want to do a single write(). */
              if (n < stream->_buf_size)
                {
                  *stream->_ptr = (char)c;
                  ++n;
                  c = -1;       /* Don't write character separately */
                }
              w = _stream_write (fh, stream->_buffer, n);
            }
          else                  /* New or flushed buffer */
            {
              PLIBCFH   pFH = __libc_FH(fh);
              w = 0;
              if (pFH && (pFH->fFlags & O_APPEND))
                lseek (fh, 0, SEEK_END);
            }
          stream->_ptr = stream->_buffer;
          stream->_wcount = stream->_buf_size;
          if (c == '\n' && (stream->_flags & _IOLBF))
            {
              ch = (char)c;
              if (_stream_write (fh, &ch, 1) != 1)
                ++n;            /* n != w */
            }
          else if (c >= 0)      /* -1: already written */
            {
              *stream->_ptr++ = (char)c;
              --stream->_wcount;
            }
        }
      else
        {
          n = 1;
          ch = (char)c;
          w = _stream_write (fh, &ch, 1);
          stream->_wcount = 0;
        }
      if (n != w)
        {
          stream->_flags |= _IOERR;
          return EOF;
        }
    }
  return 0;
}
