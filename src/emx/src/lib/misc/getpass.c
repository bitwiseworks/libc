/* getpass.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>

char *_STD(getpass) (const char *prompt)
{
  return _getpass2 (prompt, 1);
}
