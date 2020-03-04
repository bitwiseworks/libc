/*
    ungetwc.c -- copied (in part) from ungetc.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#define _GNU_SOURCE /* for _unlocked prototypes */
#define _WIDECHAR /* for getputc.h */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <emx/io.h>
#include <wchar.h>
#include "getputc.h"


wint_t _ungetwc_nolock (wint_t c, FILE *stream)
{
  if (_IS_WSPRINTF_STREAM (stream))
    {
      /* ungetc() on a string is used only by swscanf(), and this does
         an ungewtc() of the recently read character, so we don't have
         to write it to the (read-only!) string. */

      --stream->_ptr;
      ++stream->_rcount;
      stream->_flags &= ~_IOEOF;
      return c;
    }

  /* Zero character doesn't requre conversion, just call ungetc. */
  if (!c)
    return _ungetc_nolock (c, stream);

  /* Convert to multibyte and call ungetc (in reverse order!). */
  char mb[MB_LEN_MAX];
  mbstate_t state = {{0}};
  size_t cb = wcrtomb (mb, c, &state);
  if (cb == (size_t) -1)
      return WEOF;

  while (cb--)
    if (_ungetc_nolock (mb[cb], stream) == EOF)
      return WEOF;

  return c;
}


wint_t _STD(ungetwc) (wint_t c, FILE *stream)
{
  wint_t result;

  STREAM_LOCK (stream);
  result = _ungetwc_nolock (c, stream);
  STREAM_UNLOCK (stream);
  return result;
}
