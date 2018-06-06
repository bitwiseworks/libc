/* _fbuf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
                     -- Copyrithg (c) 2003 by knut st. osmundsen */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>
#include <emx/umalloc.h>

/* Assign a buffer to a stream (which must not have a buffer) */

void _fbuf (FILE *stream)
{
  if (stream->_flags & _IONBF)
    stream->_buffer = NULL;
  else
    {
      PLIBCFH   pFH = __libc_FH(stream->_handle);
      if (pFH && (   (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_DEV
                  || (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_SOCKET))
        stream->_buffer = _lmalloc(BUFSIZ);
      else
        stream->_buffer = _hmalloc(BUFSIZ);
    }
  if (stream->_buffer == NULL)
    {
      stream->_buf_size = 1;
      stream->_buffer = &stream->_char_buf;
      stream->_flags &= ~(_IOFBF|_IOLBF|_IOBUFMASK);
      stream->_flags |= _IONBF|_IOBUFCHAR;
    }
  else
    {
      stream->_buf_size = BUFSIZ;
      stream->_flags &= ~_IOBUFMASK;
      stream->_flags |= _IOBUFLIB;
    }
  stream->_ptr = stream->_buffer;
  stream->_rcount = 0;
  stream->_wcount = 0;
  stream->_ungetc_count = 0;
}
