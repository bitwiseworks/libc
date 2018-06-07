/* fnisabs.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <ctype.h>

int _fnisabs (const char *name)
{
  if (isalpha ((unsigned char)name[0]) && name[1] == ':')
    name += 2;
  return name[0] == '/' || name[0] == '\\';
}
