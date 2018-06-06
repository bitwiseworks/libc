/* fflush.c (emx+gcc) -- Copyright (c) 1990-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <emx/io.h>

int _STD(fflush_unlocked) (FILE *stream)
{
  int result, n, ft, saved_errno;
  off_t pos;

  result = 0;
  if (stream->_flags & _IOSPECIAL)
    {
      if (stream->_flush != NULL)
        result = stream->_flush (stream, _FLUSH_FLUSH);
      stream->_ungetc_count = 0; /* Undo ungetc() */
      stream->_flags &= ~_IOUNGETC;
      return result;
    }
  if ((stream->_flags & _IOWRT) && bbuf (stream))
    {
      n = stream->_ptr - stream->_buffer;
      if (n > 0 && _stream_write (stream->_handle, stream->_buffer, n) <= 0)
        {
          stream->_flags |= _IOERR;
          result = EOF;
        }
    }

  /* Undo ungetc() now, before calling ftell(). */

  if (stream->_ungetc_count != 0)
    {
      stream->_rcount = -stream->_rcount;
      stream->_ungetc_count = 0;
    }
  stream->_flags &= ~_IOUNGETC;

  saved_errno = errno;
  if ((stream->_flags & _IOREAD) && bbuf (stream) &&
      ioctl (stream->_handle, FGETHTYPE, &ft) >= 0 && ft == HT_FILE)
    {
      /* ISO 9899-1990, 7.9.5.2: "The fflush function returns EOF if a
         write error occurs, otherwise zero." */
      pos = _ftello_unlocked (stream);
      if (pos != -1)
        lseek (stream->_handle, pos, SEEK_SET);
    }
  errno = saved_errno;
  stream->_ptr = stream->_buffer;
  stream->_rcount = 0;
  stream->_wcount = 0;
  if ((stream->_flags & (_IORW|_IOWRT)) == (_IORW|_IOWRT))
    stream->_flags &= ~_IOWRT;
  return result;
}


int _STD(fflush) (FILE *stream)
{
  int result, n;
  struct streamvec *sv;

  result = 0;
  if (stream == NULL)
    {
      for (sv = _streamvec_head; sv != NULL; sv = sv->pNext)
        for (n = 0; n < sv->cFiles; ++n)
          if ((sv->aFiles[n]._flags & (_IOOPEN|_IOWRT)) == (_IOOPEN|_IOWRT))
            if (fflush (&sv->aFiles[n]) != 0)
              result = EOF;
      return result;
    }

  STREAM_LOCK (stream);
  result = fflush_unlocked (stream);
  STREAM_UNLOCK (stream);
  return result;
}
