/* _strncpy.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes */

#include <string.h>

/* mkstd.awk: NOUNDERSCORE(strncpy) */
char *_strncpy (char *string1, const char *string2, size_t size)
{
  if (size > 0)
    {
      while (size > 1 && *string2 != 0)
        {
          *string1++ = *string2++;
          --size;
        }
      *string1 = 0;
    }
  return string1;
}
