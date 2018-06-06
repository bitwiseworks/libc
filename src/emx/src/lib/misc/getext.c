/* getext.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <InnoTekLIBC/locale.h>

#define FALSE   0
#define TRUE    1

char *_getext (const char *path)
{
  int sep, mbcl;
  const char *dotp;

  sep = TRUE; dotp = NULL;
  while (*path != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *path, mbcl))
      {
        if (path[1] == 0)       /* Invalid DBCS character */
          break;
        path += mbcl;
        sep = FALSE;
      }
    else
      switch (*path++)
        {
        case '.':
          /* Note that PATH has been incremented. */
          dotp = (sep ? NULL : path - 1);
          sep = FALSE;
          break;
        case ':':
        case '/':
        case '\\':
          dotp = NULL;
          sep = TRUE;
          break;
        default:
          sep = FALSE;
          break;
        }
  return (char *)dotp;
}
