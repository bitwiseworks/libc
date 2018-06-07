/* _fopen.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <emx/io.h>

#define FALSE   0
#define TRUE    1

FILE *_openstream (FILE *dst, const char *fname, const char *mode, int shflag,
                   int lock)
{
  char ok, bt;
  int omode;

  switch (*mode)
    {
    case 'r':
      dst->_flags = _IOREAD;
      omode = O_RDONLY;
      break;
    case 'w':
      dst->_flags = _IOWRT;
      omode = O_WRONLY|O_CREAT|O_TRUNC;
      break;
    case 'a':
      dst->_flags = _IOWRT;
      omode = O_WRONLY|O_CREAT|O_APPEND;
      break;
    default:
      errno = EINVAL;
      if (lock)
        STREAMV_LOCK;
      _closestream (dst);
      if (lock)
        STREAMV_UNLOCK;
      return NULL;
    }
  ++mode; ok = TRUE; bt = FALSE;
  while (*mode != 0 && ok)
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
          if (dst->_flags & _IORW)
            ok = FALSE;
          else
            {
              omode &= ~(O_RDONLY|O_WRONLY);
              omode |= O_RDWR;
              dst->_flags &= ~(_IOREAD|_IOWRT);
              dst->_flags |= _IORW;
            }
          break;
        default:
          ok = FALSE; break;
        }
      if (ok) ++mode;
    }
  dst->_flags |= _IOOPEN | _IOBUFNONE;
  dst->_flush = _flushstream;
  if (_fmutex_create2 (&dst->__u.__fsem, 0, "LIBC stream fopen") != 0)
    {
      if (lock)
        STREAMV_LOCK;
      _closestream (dst);
      if (lock)
        STREAMV_UNLOCK;
      return NULL;
    }
  dst->_handle = sopen (fname, omode, shflag, 0644);
  if (dst->_handle < 0)
    {
      if (lock)
        STREAMV_LOCK;
      _closestream (dst);
      if (lock)
        STREAMV_UNLOCK;
      return NULL;
    }
  return dst;
}
