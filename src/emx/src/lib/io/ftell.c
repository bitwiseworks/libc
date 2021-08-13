/* ftell.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <emx/io.h>

off_t _ftello_unlocked (FILE *stream)
{
  off_t     pos;
  PLIBCFH   pFH;

  if (stream->_flags & _IOSPECIAL)
    pos = 0;
  else
    {
      pos = tell (stream->_handle);
      if (pos == -1)
        return -1;
    }
  if (stream->_flags & _IOWRT)
    {
      if (_bbuf (stream))
        {
          pos += stream->_ptr - stream->_buffer;
          if (!(stream->_flags & _IOSPECIAL)
              && (pFH = __libc_FH (stream->_handle)) != NULL
              && (pFH->fFlags & O_TEXT))
            {
              const char *p;
              int n;

              /* In text mode, newlines are translated to CR/LF pairs.
                 Adjust the position to take account of this. */

              n = stream->_ptr - stream->_buffer;
              p = stream->_buffer;
              while (n > 0)
                {
                  if (*p ++ == '\n')
                    ++pos;
                  --n;
                }
            }
        }
    }
  else if (stream->_flags & _IOREAD)
    {
      /* Subtract the number of unprocessed buffered characters from
         the file position of the end of the buffer to get the current
         position. */

      if (!(stream->_flags & _IOSPECIAL)
          && stream->_ungetc_count == 0
          && (pFH = __libc_FH (stream->_handle)) != NULL
          && (pFH->fFlags & F_CRLF))
        {
          const char *p;
          int n;

          /* There has been at least one CR/LF to newline translation.
             Assume that all newlines in the buffer have been
             translated from a CR/LF pair each. */

          p = stream->_ptr;
          n = stream->_rcount;
          pos -= n;
          while (n > 0)
            {
              if (*p++ == '\n')
                --pos;
              --n;
            }
        }
      else if (stream->_ungetc_count != 0)
        {
          /* There are characters pushed back by ungetc().  For binary
             streams, each successful ungetc() decrements the file position
             indicator.  Treat text mode like binary mode as the file
             position indicator is undefined in text mode after
             calling ungetc(). */

          pos = pos + stream->_rcount - stream->_ungetc_count;
        }
      else
        {
          /* Binary mode. */

          pos -= stream->_rcount;
        }
    }
  return pos;
}

__off_t _STD(ftello) (FILE *stream)
{
  off_t off;

  STREAM_LOCK (stream);
  off = _ftello_unlocked (stream);
  STREAM_UNLOCK (stream);
  return off;
}

long _STD(ftell) (FILE *stream)
{
  off_t off = ftello (stream);
#if OFF_MAX > LONG_MAX
  if (off > LONG_MAX)
    {
      errno = EOVERFLOW;
      return -1;
    }
  return (long)off;
#else
  return off;
#endif
}
