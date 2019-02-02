/* freopen.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <share.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/io.h>

#define FALSE   0
#define TRUE    1

/**
 * Interprets the stream mode string.
 *
 * @returns the basic stream flags and *pmode set to the open mode.
 * @returns -1 and *pomode unset on failure.
 * @param   mode    The mode string.
 * @param   pomode  Where to store the open mode.
 */
static int _interpret_stream_mode(const char *mode, int *pomode)
{
    int flags = 0;
    int omode = 0;
    char bt, ok;

    switch (*mode)
      {
      case 'r':
        flags = _IOREAD;
        omode = O_RDONLY;
        break;
      case 'w':
        flags = _IOWRT;
        omode = O_WRONLY|O_CREAT|O_TRUNC;
        break;
      case 'a':
        flags = _IOWRT;
        omode = O_WRONLY|O_CREAT|O_APPEND;
        break;
      default:
        errno = EINVAL;
        return -1;
      }

    ok = TRUE;
    bt = FALSE;
    while (ok && *++mode)
      {
        switch (*mode)
          {
          case 't':
            if (bt)
              ok = FALSE;
            else
              {
                bt = TRUE;
                omode |= O_TEXT;
              }
            break;
          case 'b':
            if (bt)
              ok = FALSE;
            else
              {
                bt = TRUE;
                omode |= O_BINARY;
              }
            break;
          case '+':
            if (flags & _IORW)
              ok = FALSE;
            else
              {
                omode &= ~(O_RDONLY|O_WRONLY);
                omode |= O_RDWR;
                flags &= ~(_IOREAD|_IOWRT);
                flags |= _IORW;
              }
            break;
          default:
            ok = FALSE;
            break;
          }
      }
    *pomode = omode;
    return flags;
}

FILE *_STD(freopen) (const char *fname, const char *mode, FILE *stream)
{
  FILE *result = NULL;

  STREAMV_LOCK;
  if (!fname)
    {
      /*
       * Change the stream mode.
       */
      if (stream->_flags & _IOOPEN)
        {
          int omode;
          int flags = _interpret_stream_mode(mode, &omode);
          if (flags != -1)
            {
              if (   ((flags & _IORW)   && !(stream->_flags & _IORW))
                  || ((flags & _IOREAD) && !(stream->_flags & (_IORW | _IOREAD)))
                  || ((flags & _IOWRT)  && !(stream->_flags & (_IORW | _IOWRT)))
                  )
                  errno = EINVAL;
              else
                {
                  if (!(stream->_flags & _IOSPECIAL))
                    {
                      /* flush it and set the new mode */
                      fflush(stream);
                      if (!fcntl(fileno(stream), F_SETFL, omode))
                        stream = stream;
                    }
                  else
                    errno = EBADF; /* doesn't support the mode. */
                }
            }
        }
      else
        errno = EBADF;
    }
  else
    {
      if (stream->_flags & _IOOPEN)
        { /* duplication of fclose(), but no _closestream lock. */
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
          _closestream (stream);
        }
      result = _openstream (stream, fname, mode, SH_DENYNO, 0);
    }
  STREAMV_UNLOCK;
  return result;
}
