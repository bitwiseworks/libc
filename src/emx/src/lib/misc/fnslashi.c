/* fnslashi.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <InnoTekLIBC/locale.h>

char *_fnslashify (char *name)
{
  char *p;
  int mbcl;

  p = name;
  while (*p != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *p, mbcl) && p[1] != 0)
      p += mbcl;
    else if (*p == '\\')
      *p++ = '/';
    else
      ++p;
  return name;
}
