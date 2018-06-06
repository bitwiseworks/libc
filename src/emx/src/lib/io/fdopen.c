/* fdopen.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdio.h>
#include <emx/io.h>

#define FALSE   0
#define TRUE    1

/* Bug: doesn't check for compatible modes (O_ACCMODE) */

FILE *_STD(fdopen) (int handle, const char *mode)
{
  char ok, bt;
  FILE *dst;
  int omode;

  if (!__libc_FH(handle))
    {
      errno = EBADF;
      return NULL;
    }
  dst = _newstream ();
  if (!dst)
    return NULL;
  if (_fmutex_create2 (&dst->__u.__fsem, 0, "LIBC stream fdopen") != 0)
    {
      STREAMV_LOCK;
      _closestream (dst);
      STREAMV_UNLOCK;
      return NULL;
    }
  switch (*mode)
    {
    case 'r':
      dst->_flags = _IOREAD;
      break;
    case 'w':
      dst->_flags = _IOWRT;
      break;
    case 'a':
      dst->_flags = _IOWRT;
      break;
    default:
      STREAMV_LOCK;
      _closestream (dst);
      STREAMV_UNLOCK;
      return NULL;
    }
  ++mode; ok = TRUE; bt = FALSE; omode = 0;
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
              omode = O_TEXT;
            }
          break;
        case 'b':
          if (bt)
            ok = FALSE;
          else
            {
              bt = TRUE;
              omode = O_BINARY;
            }
          break;
        case '+':
          if (dst->_flags & _IORW)
            ok = FALSE;
          else
            {
              dst->_flags &= ~(_IOREAD|_IOWRT);
              dst->_flags |= _IORW;
            }
          break;
        default:
          ok = FALSE; break;
        }
      if (ok) ++mode;
    }
  if (bt)
    setmode (handle, omode);
  dst->_handle = handle;
  dst->_flags |= _IOOPEN | _IOBUFNONE;
  dst->_flush = _flushstream;
  return dst;
}
