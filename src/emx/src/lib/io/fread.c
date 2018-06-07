/* fread.c (emx+gcc) -- Copyright (c) 1990-1999 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <emx/io.h>

size_t _STD(fread)(void *buffer, size_t size, size_t count, FILE *stream)
{
    STREAM_LOCK(stream);
    size_t cb = fread_unlocked(buffer, size, count, stream);
    STREAM_UNLOCK(stream);
    return cb;
}

size_t _STD(fread_unlocked)(void *buffer, size_t size, size_t count, FILE *stream)
{
  size_t total, left, n;
  int r;   /* signed! */
  char *dst;
  int fh, c;

  if (size == 0 || count == 0)
    return 0;
  total = size * count;
  if (total / count != size)
    {
      errno = ERANGE;
      return 0;
    }

  /* check that this stream can be read. */
  if ((stream->_flags & (_IOREAD | _IORW | _IOWRT)) == _IOWRT)
    {
      stream->_flags |= _IOERR;
      errno = EACCES;
      return 0;
    }

  if (nbuf (stream))
    _fbuf (stream);
  left = total;
  dst = buffer;
  fh = stream->_handle;
  stream->_flags |= _IOREAD;
  if (bbuf (stream))
    while (left != 0)
      {
        if (stream->_rcount > 0)
          {
            n = MIN ((size_t)stream->_rcount, left);
            memcpy (dst, stream->_ptr, n);
            stream->_ptr += n;
            stream->_rcount -= n;
            dst += n;
            left -= n;
          }
        else if (left > BUFSIZ && stream->_ungetc_count == 0)
          {
            n = (left / BUFSIZ) * BUFSIZ;      /* Number of buffers */
            r = _stream_read (fh, dst, n);
            if (r < 0)
              {
                stream->_flags |= _IOERR;
                break;
              }
            if (r == 0)
              {
                stream->_ptr = stream->_buffer;
                stream->_flags |= _IOEOF;
                if (stream->_flags & _IORW)
                  stream->_flags &= ~(_IOREAD|_IOWRT);
                break;
              }
            left -= r;
            dst += r;
          }
        else
          {
            --stream->_rcount;
            c = _fill (stream);
            if (c == EOF)
              break;
            *dst++ = (char)c;
            --left;
          }
      }
  else
    {
      if (stream->_rcount > 0)
        {
          n = MIN ((size_t)stream->_rcount, left);
          memcpy (dst, stream->_ptr, n);
          stream->_ptr += n;
          stream->_rcount -= n;
          dst += n;
          left -= n;
        }
      while (left != 0)
        {
          if (stream->_ungetc_count != 0)
            {
              --stream->_rcount;
              c = _fill (stream);
              if (c == EOF)
                break;
              *dst++ = (char)c;
              --left;
            }
          else
            {
              r = _stream_read (fh, dst, left);
              if (r < 0)
                {
                  stream->_flags |= _IOERR;
                  break;
                }
              if (r == 0)
                {
                  stream->_ptr = stream->_buffer;
                  stream->_flags |= _IOEOF;
                  if (stream->_flags & _IORW)
                    stream->_flags &= ~(_IOREAD|_IOWRT);
                  break;
                }
              dst += r;
              left -= r;
            }
        }
    }
  return (total - left) / size;
}
