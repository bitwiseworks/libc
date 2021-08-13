/* ungetc.c (emx+gcc) -- Copyright (c) 1990-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <emx/io.h>

/* The implementation of ungetc() has been changed in emx 0.9a fix04
   to fix bugs.  The new implementation adds the `_ungetc_count' field
   to `struct __sFILE'.  Unfortunately, the new implementation must
   still work with the old getc().  Otherwise, old programs using the
   new dynamic library, and old modules (compiled with the inline
   getc() of the old stdio.h) linked with the new static library would
   break.

   As `_ungetc_count' must be adjusted for every evaluation of getc()
   while there are pushed-back characters, getc() must call _fill() if
   there are pushed-back characters.  Checking `_ungetc_count' is not
   an option for the reasons given above.  The only way to make the
   old getc() call _fill() is making `rcount' less than 1.

   If we just set `rcount' to, say, 0, if there are pushed-back
   characters, we have to refill the buffer after all pushed-back
   characters have been read, as the original `rcount' is no longer
   available (and we don't have space in the structure for saving the
   old value).  That is, the buffer will be flushed after ungetc().
   That's too inefficient.

   Therefore, we encode the number of characters available for reading
   in the stream buffer as negative number in the `rcount' field while
   there are pushed-back characters.  We use the GET_RCOUNT macro to
   retrieve the number of characters available for reading.

   This design also works for executables built with the new stdio.h
   and linked with the old library (this should happen only when
   linking dynamically) as getc() has not been changed at all.

   Programs which directly use `rcount' will break. */


int _ungetc_nolock (int c, FILE *stream)
{
  if (!(stream->_flags & _IOOPEN) || c == EOF)
    return EOF;
  if ((stream->_flags & _IOSPECIAL) && stream->_flush == NULL)
    {
      /* ungetc() on a string is used only by sscanf(), and this does
         an ungetc() of the recently read character, so we don't have
         to write it to the (read-only!) string. */

      --stream->_ptr;
      ++stream->_rcount;
      stream->_flags &= ~_IOEOF;
      return (unsigned char)c;
    }

  /* Fail if the stream is in write mode. */

  if (stream->_flags & _IOWRT)
    {
      stream->_flags |= _IOERR;
      errno = EACCES;
      return EOF;
    }

  /* Switch to read mode.  Note that _fbuf() zeros `rcount' and
     `_ungetc_count'.  We call it now to simplify things. */

  stream->_flags |= _IOREAD;
  stream->_wcount = 0;
  if (_nbuf (stream))
    _fbuf (stream);

  /* Check `_ungetc_count' for overflow. */

  if (stream->_ungetc_count == UCHAR_MAX)
    return EOF;

  /* Check for buffer overflow. */

  if (stream->_ungetc_count >= stream->_buf_size)
    return EOF;

  /* Push back the character by overwriting a character in the stream
     buffer before `ptr'.  There is always at least one character
     available: The only case where `ptr' equals `buffer' and there is
     data in the buffer is created fseek().  If fseek() has succeeded,
     the underlying file must be a disk file, therefore we can safely
     flush the buffer to make space.  This ensures one character of
     pushback. */

  if (stream->_rcount != 0
      && stream->_ptr - stream->_buffer < stream->_ungetc_count + 1)
    {
      int ft, uc;

      /* If there are not enough characters available before `ptr',
         we'll flush the buffer to make space.  However, we must be
         able to reread after undoing ungetc() to avoid data loss.
         Therefore, fail now if we have to flush the buffer and there
         is data in the buffer and the underlying file is not a disk
         file.  (This can happen only if at least one character is
         currently pushed back, see above.) */

      if ((stream->_flags & _IOSPECIAL)
          || ioctl (stream->_handle, FGETHTYPE, &ft) < 0
          || ft != HT_FILE)
        return EOF;

      /* OK, we can flush the stream buffer.  We flush the buffer
         completely and don't attempt to keep some data (that would be
         difficult w.r.t. newlines in text mode).  Note that fflush()
         does not modify the EOF indicator and the error indicator. */

      uc = stream->_ungetc_count;
      if (fflush_unlocked (stream) != 0)
        return EOF;
      stream->_ungetc_count = uc;
    }

  /* Now there is enough space available.  We insert the characters
     beginning at the low end of the buffer to avoid various special
     cases related to the value of `ptr'. */

  stream->_buffer[stream->_ungetc_count] = (char)c;

  /* Set a flag indicating that we have modified the buffer. */

  stream->_flags |= _IOUNGETC;

  /* Let `getc()' call _fill(), and save the value of `rcount'.
     Increment the `_ungetc_count' field which contains the number of
     characters pushed back. */

  if (stream->_ungetc_count == 0)
    stream->_rcount = -stream->_rcount;
  stream->_ungetc_count += 1;

  /* Clear any EOF condition. */

  stream->_flags &= ~_IOEOF;

  return (unsigned char)c;
}


int _STD(ungetc) (int c, FILE *stream)
{
  int result;

  STREAM_LOCK (stream);
  result = _ungetc_nolock (c, stream);
  STREAM_UNLOCK (stream);
  return result;
}
