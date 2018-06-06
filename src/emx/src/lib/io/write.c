/* write.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <signal.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <emx/io.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/backend.h>

#define CTRL_Z 0x1a

#define BEGIN do {
#define END   } while (0)

/* Write the current buffer, for write_text(). */

#define WRTBUF                                                  \
  BEGIN                                                         \
    n = __write (handle, buf, buf_cnt);                         \
    if (n < 0) return -1;                                       \
    out_cnt += n;                                               \
    if (n != buf_cnt) goto partial;                             \
    buf_cnt = 0; lf_cnt_1 = lf_cnt; buf_pos = i;                \
  END

/* Write to a text-mode file.  Partial writes are possible.  `buf' is
   big enough for supporting atomic writes for

        NBYTE <= PIPE_SIZE = 512

   and any contents of the buffer pointed to by SRC (the worst case is
   all newlines).  Return the number of characters (of SRC) written.
   Return -1 on error. */

static int write_text (int handle, const char *src, size_t nbyte, PLIBCFH pFH)
{
  int out_cnt, lf_cnt, lf_cnt_1, buf_cnt, n;
  size_t i, buf_pos;
  char buf[2*512];
  char splitp;

  out_cnt = lf_cnt = lf_cnt_1 = buf_cnt = 0;
  buf_pos = 0; i = 0;
  if (pFH->fFlags & F_WRCRPEND)
    {
      if (*src == '\n')
        {
          /* Handle the first newline specially: Don't prepend a '\r'
             character. */

          buf[buf_cnt++] = src[i++];
        }
      else
        {
          /* This is the case where our algorithm fails: The
             application does not try to continue writing with a
             newline.  There will be an unexpected '\r' left in the
             pipe. */

          pFH->fFlags &= ~F_WRCRPEND;
        }
    }
  while (i < nbyte)
    {
      if (src[i] == '\n')
        {
          /* Don't split '\r' and '\n' between two __write() calls as
             we cannot remove the '\r' from a pipe in case the second
             __write() fails. */

         if (buf_cnt >= sizeof (buf) - 1) WRTBUF;
          buf[buf_cnt++] = '\r';
          ++lf_cnt;
        }
      if (buf_cnt >= sizeof (buf)) WRTBUF;
      buf[buf_cnt++] = src[i++];
    }
  if (buf_cnt != 0) WRTBUF;
  pFH->fFlags &= ~F_WRCRPEND;
  return out_cnt - lf_cnt;

partial:

  /* Not all bytes have been written. */

  if (n == 0)
    return out_cnt - lf_cnt_1;

  /* Adjust the return value for newline conversion: Add to `lf_cnt_1'
     the number of '\r' characters written.

     If the '\r', but not the '\n', of an expanded newline has been
     written, processing depends on the handle type.  For files, we
     remove the '\r' by changing the size of the file.  For pipes (and
     devices), we exclude the newline from the count and set a flag to
     let the next write() omit the '\r' if the first character to be
     written is a newline. */

  buf_cnt = 0; splitp = 0;
  for (i = buf_pos; buf_cnt < n; ++i)
    {
      /* Check for '\r' in `buf' to handle the case of an initial '\n'
         with suppressed '\r'. */

      if (src[i] == '\n' && buf[buf_cnt] == '\r')
        {
          ++buf_cnt;
          if (buf_cnt == n)
            splitp = 1;
          if (buf_cnt <= n)
            ++lf_cnt_1;
        }
      ++buf_cnt;
    }

  if (splitp)
    {
      if ((pFH->fFlags & __LIBC_FH_TYPEMASK) != F_FILE)
        pFH->fFlags |= F_WRCRPEND;
      else
        {
          off_t pos = __libc_Back_ioSeek (handle, -1, SEEK_CUR);
          if (pos >= 0)
            __libc_Back_ioFileSizeSet (handle, pos, 0);
        }
    }
  else
    pFH->fFlags &= ~F_WRCRPEND;
  return out_cnt - lf_cnt_1;
}


int _STD(write) (int handle, const void *buf, size_t nbyte)
{
  PLIBCFH   pFH;
  int       n;
  const char *src;

  /*
   * Get filehandle
   */
  pFH = __libc_FH(handle);
  if (!pFH)
      return -1;

  if ((pFH->fFlags & (__LIBC_FH_TYPEMASK | O_APPEND)) == (O_APPEND | F_FILE))
    __libc_Back_ioSeek (handle, 0, SEEK_END);
  if (nbyte == 0)                 /* Avoid truncation of file */
    return 0;
  src = buf;
  if (    (pFH->fFlags & O_TEXT)
       && (   (pFH->fFlags & F_WRCRPEND)
           || memchr (src, '\n', nbyte) != NULL))
    n = write_text (handle, src, nbyte, pFH);
  else
    n = __write (handle, src, nbyte);

  if (n < 0)
    {
      if (errno == EPIPE)
        {
          raise (SIGPIPE);
          errno = EPIPE;
        }
      return -1;
    }
  if (n == 0)
    {
      /* Ctrl-Z cannot be written to certain devices (printer) unless
         they have been switched to binary mode.  Don't return an
         error if writing failed due to a Ctrl-Z at the start of the
         data to be written. */

      if (   (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_DEV
          && *src == CTRL_Z)
        return 0;

      /* For devices and pipes with O_NONBLOCK set, set errno to
         EAGAIN. */

      if (   (pFH->fFlags & O_NONBLOCK)
          && (   (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_DEV
              || (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_SOCKET /* ???? */
              || (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_PIPE) )
        {
          errno = EAGAIN;
          return -1;
        }

      /* Set errno to ENOSPC in all remaining cases (disk full while
         writing to a disk file). */

      errno = ENOSPC;
      return -1;
    }
  return n;
}
