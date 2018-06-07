/* eof.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <errno.h>
#include <emx/io.h>

int _STD(eof) (int handle)
{
  PLIBCFH   pFH;
  off_t cur, len;

  /*
   * Get filehandle.
   */
  pFH = __libc_FH(handle);
  if (!pFH)
  {
      errno = EBADF;
      return -1;
  }
  if (pFH->fFlags & F_EOF)          /* Ctrl-Z reached */
    return 1;
  cur = tell (handle);
  if (cur < 0)
    return -1;
  len = filelength (handle);
  if (len < 0)
    return -1;
  return cur == len;
}
