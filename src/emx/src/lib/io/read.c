/* read.c (emx+gcc) -- Copyright (c) 1990-1999 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/builtin.h>
#include <emx/io.h>
#include <emx/syscalls.h>

/* Read NBYTE characters, use lookahead, if available. This is simple
   unless O_NDELAY is in effect. */

static int read_lookahead (int handle, void *buf, size_t nbyte, PLIBCFH pFH)
{
  int i, n, la, saved_errno;
  char *dst;

  i = 0; dst = buf; saved_errno = errno;
  if (nbyte > 0 && (la = __lxchg (&pFH->iLookAhead, -1)) != -1)
    {
      *dst = (char)la;
      ++i; --nbyte;
    }
  n = __read (handle, dst+i, nbyte);
  if (n == -1)
    {
      if (errno == EAGAIN && i > 0)           /* lookahead and O_NDELAY */
        {
          errno = saved_errno;                /* hide EAGAIN */
          return i;                           /* and be successful */
        }
      return -1;
    }
  return i + n;
}


ssize_t _STD(read) (int handle, void *buf, size_t nbyte)
{
  int       n;
  PLIBCFH   pFH;
  size_t    j, k;
  char     *dst, c;

  /*
   * Get filehandle.
   */
  pFH = __libc_FH(handle);
  if (!pFH)
    {
      errno = EBADF;
      return -1;
    }

  pFH->fFlags &= ~F_CRLF;           /* No CR/LF pair translated to newline */
  if (nbyte > 0 && (pFH->fFlags & F_EOF))
    return 0;
  dst = buf;
  n = read_lookahead (handle, dst, nbyte, pFH);
  if (n == -1)
    return -1;
  if ((pFH->fFlags & O_TEXT) && !(pFH->fFlags & F_TERMIO) && n > 0)
    {
      /* special processing for text mode */
      if (  (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_FILE
          && dst[n-1] == 0x1a
          && eof (handle))
        {
          /* remove last Ctrl-Z in text files */
          --n;
          pFH->fFlags |= F_EOF;
          if (n == 0)
            return 0;
        }
      if (n == 1 && dst[0] == '\r')
        {
          /* This is the tricky case as we are not allowed to
             decrement n by one as 0 indicates end of file. We have to
             use look ahead. */

          int saved_errno = errno;
          j = read_lookahead (handle, &c, 1, pFH); /* look ahead */
          if (j == -1 && errno == EAGAIN)
            {
              pFH->iLookAhead = '\r';
              return -1;
            }
          errno = saved_errno;                /* hide error */
          if (j == 1 && c == '\n')            /* CR/LF ? */
            {
              dst[0] = '\n';                  /* yes -> replace with LF */
              pFH->fFlags |= F_CRLF;
            }
          else
            pFH->iLookAhead = (unsigned char)c; /* no -> save the 2nd char */
        }
      else
        {
          /* Translate each CR/LF pair to a newline character.  Set
             F_CRLF if at lest one such translation has been
             performed.  Set the file's look-ahead to CR if the buffer
             ends with CR. */

          if (_crlf (dst, n, &k))
            {
              /* Buffer ends with CR: Set look ahead and adjust `n'
                 for F_CRLF logic. */

              pFH->iLookAhead = '\r';
              --n;
            }
          if (k != n)
            pFH->fFlags |= F_CRLF;
          n = k;
        }
    }
  return n;
}
