/* fwrite.c (emx+gcc) -- Copyright (c) 1990-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <emx/io.h>


static size_t fwrite_internal (const char *src, size_t left, FILE *stream)
{
  size_t n;
  int w;   /* signed! */
  int fh;

  fh = stream->_handle;
  if (stream->_flags & _IOSPECIAL)
    {
      while (left != 0)
        {
          n = stream->_wcount;
          if (n == 0)
            {
              if (_flush (*src++, stream) == EOF)
                {
                  stream->_flags |= _IOERR;
                  return left;
                }
              --left;
            }
          else
            {
              if (left < n)
                n = left;
              memcpy (stream->_ptr, src, n);
              stream->_ptr += n; stream->_wcount -= n;
              src += n; left -= n;
            }
        }
    }
  else if (_bbuf (stream))
    {
      if (stream->_wcount == 0 && stream->_ptr == stream->_buffer)
        stream->_wcount = stream->_buf_size;
      while (left != 0)
        {
          if (left <= stream->_wcount)
            {
              memcpy (stream->_ptr, src, left);
              stream->_ptr += left;
              stream->_wcount -= left;
              src += left;
              left = 0;
            }
          else if (stream->_ptr != stream->_buffer) /* Buffer non-empty */
            {
              n = stream->_ptr - stream->_buffer;
              stream->_ptr = stream->_buffer;
              while (n != 0)
                {
                  w = _stream_write (fh, stream->_ptr, n);
                  if (w <= 0)
                    {
                      stream->_flags |= _IOERR;
                      return left;
                    }
                  n -= w; stream->_ptr += w;
                }
              stream->_ptr = stream->_buffer;
              stream->_wcount = stream->_buf_size;
            }
          else
            {
              w = _stream_write (fh, src, left);
              if (w <= 0)
                {
                  stream->_flags |= _IOERR;
                  return left;
                }
              src += w;
              left -= w;
            }
        }
    }
  else
    while (left != 0)
      {
        w = _stream_write (fh, src, left);
        if (w <= 0)
          {
            stream->_flags |= _IOERR;
            return left;
          }
        src += w;
        left -= w;
      }
  return left;
}

size_t _STD(fwrite_unlocked) (const void *buffer, size_t size, size_t count, FILE *stream)
{
  size_t total, left, n;
  const char *src, *newline;

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
  if (size == 0 || count == 0)
    return 0;
  total = size * count;
  if (total / count != size)
    {
      errno = ERANGE;
      return 0;
    }

  if (_nbuf (stream))
    _fbuf (stream);
  src = buffer;
  stream->_flags |= _IOWRT;

  if (stream->_flags & _IOLBF)
    {
      newline = _memrchr (src, '\n', total);
      if (newline == NULL)
        left = fwrite_internal (src, total, stream);
      else
        {
          n = (size_t)(newline - src) + 1;
          left = fwrite_internal (src, n, stream) + total - n;
          if (left == total - n)
            {
              /* Note that the return value will be inaccurate if
                 fflush_unlocked() fails, but that's also true for
                 _IOFBF. */

              if (fflush_unlocked (stream) != 0)
                left = total;
              else if (left != 0)
                left = fwrite_internal (src + n, left, stream);
            }
        }
    }
  else
    left = fwrite_internal (src, total, stream);
  return (total - left) / size;
}


size_t _STD(fwrite) (const void *buffer, size_t size, size_t count, FILE *stream)
{
  size_t r;

  STREAM_LOCK (stream);
  r = fwrite_unlocked (buffer, size, count, stream);
  STREAM_UNLOCK (stream);
  return r;
}
