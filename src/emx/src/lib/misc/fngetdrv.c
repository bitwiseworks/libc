/* fngetdrv.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>

char _fngetdrive (const char *src)
{
  /* As we check the first character we can't hit by accident a DBCS
     character having ':' as second byte. */

  if (src[0] >= 'A' && src[0] <= 'Z' && src[1] == ':')
    return src[0];
  else if (src[0] >= 'a' && src[0] <= 'z' && src[1] == ':')
    return src[0]-'a'+'A';
  else
    return 0;
}
