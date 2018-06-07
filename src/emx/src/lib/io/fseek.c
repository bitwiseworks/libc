/* fseek.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <errno.h>
#include <emx/io.h>

int _fseek_unlocked (FILE *stream, off_t offset, int origin)
{
  off_t     cur_pos;
  int       fflush_result;
  PLIBCFH   pFH;

  if (!(stream->_flags & _IOOPEN) || origin < 0 || origin > 2
      || (stream->_flags & _IOSPECIAL))
    {
      errno = EINVAL;
      return EOF;
    }

  cur_pos = -1;                 /* Not yet computed */

  /* fflush() is not required if all of the following conditions are met:
     - the stream is a read-only file
     - the buffer has not been modified by ungetc()
     - the stream is buffered
     - there are no pushed-back characters
     - the new position is within buffer */

  if ((stream->_flags & (_IORW|_IOREAD|_IOWRT|_IOUNGETC)) == _IOREAD
      && bbuf (stream) && stream->_ungetc_count == 0)
    {
      off_t file_pos, end_pos, buf_pos;
      int text_mode, n;

      file_pos = tell (stream->_handle);
      if (file_pos == -1) return EOF;
      cur_pos = _ftello_unlocked (stream);
      if (origin == SEEK_CUR)
        {
          offset += cur_pos;
          origin = SEEK_SET;
        }
      else if (origin == SEEK_END)
        {
          end_pos = lseek (stream->_handle, 0, SEEK_END);
          lseek (stream->_handle, file_pos, SEEK_SET);
          if (end_pos == -1)
            return EOF;
          offset += end_pos;
          origin = SEEK_SET;
        }
      text_mode = (   (pFH = __libc_FH (stream->_handle)) != NULL
                   && (pFH->fFlags & F_CRLF));
      n = stream->_ptr - stream->_buffer;
      if (text_mode)
        {
          /* Use lower bound on the buffer position for a quick check
             if the new position can at all be inside the buffer.  The
             lower bound is the exact buffer position if all
             characters in the buffer are newline characters. */

          buf_pos = cur_pos - 2 * n;
        }
      else
        buf_pos = cur_pos - n;

      if (offset >= buf_pos && offset < file_pos)
        {
          if (text_mode)
            {
              const char *p;

              /* Compute exact buffer position.  This is only required
                 if the new position is smaller than the current
                 position. */

              if (offset >= cur_pos)
                buf_pos = cur_pos; /* Wrong value doesn't matter */
              else
                {
                  p = stream->_ptr;
                  buf_pos = cur_pos - n;
                  while (n > 0)
                    {
                      if (*--p == '\n')
                        --buf_pos;
                      --n;
                    }
                }

              if (offset >= buf_pos)
                {
                  off_t tmp_pos;

                  /* The new position is within the buffer.  Adjust
                     the new position for newline characters.  If
                     offset >= cur_pos, we can start at the current
                     position. */

                  if (offset >= cur_pos)
                    {
                      /* Optimization. */

                      p = stream->_ptr;
                      tmp_pos = cur_pos;
                    }
                  else
                    {
                      p = stream->_buffer;
                      tmp_pos = buf_pos;
                    }
                  while (tmp_pos < offset)
                    {
                      if (*p == '\n')
                        {
                          ++tmp_pos;
                          if (tmp_pos >= offset)
                            break;
                        }
                      ++p; ++tmp_pos;
                    }
                  stream->_rcount -= p - stream->_ptr;
                  stream->_ptr = (char *)p;
                  stream->_flags &= ~_IOEOF;
                  return 0;
                }
            }
          else if (offset >= buf_pos)
            {
              stream->_ptr = stream->_buffer + (offset - buf_pos);
              stream->_rcount = file_pos - offset;
              stream->_flags &= ~_IOEOF;
              return 0;
            }
        }
    }

#if 1
  /* Get file position indicator before undoing any effects of
     ungetc().  ANSI X3.159-1989 is not precise on this. */

  if (origin == SEEK_CUR && cur_pos == -1)
    cur_pos = _ftello_unlocked (stream);
#endif

  fflush_result = fflush_unlocked (stream);
  stream->_flags &= ~_IOEOF;
  if (stream->_flags & _IORW)
    stream->_flags &= ~(_IOREAD|_IOWRT);

  if (origin == SEEK_CUR)
    {
      if (cur_pos == -1)
        cur_pos = _ftello_unlocked (stream);
      if (cur_pos == -1)
        {
          /* "fseek (f, 0L, SEEK_CUR)" should not fail for
             non-seekable files as it is required for switching from
             read mode to write mode. */

          if (offset == 0 && (stream->_flags & _IORW))
            return fflush_result;
          return EOF;
        }
      offset += cur_pos;
      origin = SEEK_SET;
    }

  if (lseek (stream->_handle, offset, origin) == -1)
    return EOF;
  else
    return fflush_result;
}


int _STD(fseeko) (FILE *stream, off_t offset, int origin)
{
    int result;

    STREAM_LOCK (stream);
    result = _fseek_unlocked (stream, offset, origin);
    STREAM_UNLOCK (stream);
    return result;
}

int _STD(fseek) (FILE *stream, long offset, int origin)
{
  return fseeko (stream, (off_t)offset, origin);
}
