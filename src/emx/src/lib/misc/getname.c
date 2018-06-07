/* getname.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#define __INTERNAL_DEFS
#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <InnoTekLIBC/locale.h>

char *_getname (const char *path)
{
  const char *p;
  int mbcl;

  p = path;
  while (*path != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *path, mbcl))
      {
        if (path[1] == 0)       /* Invalid DBCS character */
          break;
        path += mbcl;
      }
    else
      switch (*path++)
        {
        case ':':
        case '/':
        case '\\':
          p = path;             /* Note that PATH has been incremented */
          break;
        }
  return (char *)p;
}
