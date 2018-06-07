/* splitpath.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#define __INTERNAL_DEFS
#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <InnoTekLIBC/locale.h>

void _splitpath (const char *src, char *drive, char *dir, char *fname,
                 char *ext)
{
  int i, j, mbcl;

  i = 0;
  while (src[i] != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, src[i], mbcl))
      {
        if (src[i+1] == 0)      /* Invalid DBCS character */
          break;
        i += mbcl;
      }
    else if (src[i] == ':')
      break;
    else
      ++i;

  if (i > 0 && i + 2 <= _MAX_DRIVE && src[i] == ':')
    {
      if (drive != NULL)
        _strncpy (drive, src, MIN (i+2, _MAX_DRIVE));
      src += i + 1;
    }
  else if (drive != NULL)
    *drive = 0;

  i = 0; j = 0;
  while (src[j] != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, src[j], mbcl))
      {
        if (src[j+1] == 0)      /* Invalid DBCS character */
          break;
        j += mbcl;
      }
    else if (src[j] == '/' || src[j] == '\\')
      i = ++j;
    else
      ++j;

  if (dir != NULL)
    _strncpy (dir, src, MIN (_MAX_DIR, i + 1));
  src += i;

  i = 0; j = 0;
  while (src[j] != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, src[j], mbcl))
      {
        if (src[j+1] == 0)      /* Invalid DBCS character */
          break;
        j += mbcl;
      }
    else if (src[j] == '.')
      i = j++;
    else
      ++j;

  if (i == 0)
    i = j;
  if (fname != NULL)
    _strncpy (fname, src, MIN (_MAX_FNAME, i + 1));
  src += i;

  if (ext != NULL)
    _strncpy (ext, src, _MAX_EXT);
}
