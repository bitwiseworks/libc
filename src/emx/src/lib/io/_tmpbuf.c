/* _tmpbuf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <emx/io.h>

/* Assign the temporary buffer BUF of size BUFSIZ to STREAM. */

int _tmpbuf1 (FILE *stream, void *buf)
{
  stream->_ptr = stream->_buffer = (char *)buf;
  stream->_wcount = BUFSIZ;
  stream->_rcount = 0;
  stream->_flags |= _IOWRT;
  stream->_flags &= ~_IOBUFMASK;
  stream->_flags |= _IOBUFTMP;
  return 0;
}

int _endbuf1 (FILE *stream)
{
  int result;

  result = fflush_unlocked (stream);
  stream->_buf_size = 1;
  stream->_ptr = stream->_buffer = &stream->_char_buf;
  stream->_flags &= ~_IOBUFMASK;
  stream->_flags |= _IOBUFCHAR;
  stream->_rcount = stream->_wcount = 0;
  return result;
}
