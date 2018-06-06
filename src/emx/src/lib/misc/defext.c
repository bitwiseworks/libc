/* defext.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <InnoTekLIBC/locale.h>

#define FALSE   0
#define TRUE    1

void _defext (char *dst, const char *ext)
{
  int dot, sep, mbcl;

  dot = FALSE; sep = TRUE;
  while (*dst != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *dst, mbcl))
      {
        if (dst[1] == 0)        /* Invalid DBCS character */
          return;
        dst += mbcl;
        sep = FALSE;
      }
    else
      switch (*dst++)
        {
        case '.':
          dot = TRUE;
          sep = FALSE;
          break;
        case ':':
        case '/':
        case '\\':
          dot = FALSE;
          sep = TRUE;
          break;
        default:
          sep = FALSE;
          break;
        }
  if (!dot && !sep)
    {
      *dst++ = '.';
      strcpy (dst, ext);
    }
}
