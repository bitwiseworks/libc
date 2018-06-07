/* makepath.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <InnoTekLIBC/locale.h>
#include <string.h>

void _makepath (char *dst, const char *drive, const char *dir,
                const char *fname, const char *ext)
{
  int n, mbcl;
  char slash, last;

  n = 0; slash = '/';
  if (drive != NULL && *drive != 0)
    {
      dst[n++] = *drive;
      dst[n++] = ':';
    }
  if (dir != NULL && *dir != 0)
    {
      last = 0;
      while (n < _MAX_PATH - 1 && *dir != 0)
        {
          if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *dir, mbcl))
            {
              if (dir[1] == 0)
                ++dir;          /* Invalid DBCS character */
              else if (n + mbcl < _MAX_PATH)
                {
                  memcpy (dst, dir, mbcl);
                  dst += mbcl; dir += mbcl;
                }
              last = 0;
            }
          else
            {
              last = *dir;
              if (*dir == '\\')
                slash = '\\';
              dst[n++] = *dir++;
            }
        }
      if (last != '\\' && last != '/' && n < _MAX_PATH - 1)
        dst[n++] = slash;
    }
  if (fname != NULL)
    {
      while (n < _MAX_PATH - 1 && *fname != 0)
        dst[n++] = *fname++;
    }
  if (ext != NULL && *ext != 0)
    {
      if (*ext != '.' && n < _MAX_PATH - 1)
        dst[n++] = '.';
      while (n < _MAX_PATH - 1 && *ext != 0)
        dst[n++] = *ext++;
    }
  dst[n] = 0;
}
