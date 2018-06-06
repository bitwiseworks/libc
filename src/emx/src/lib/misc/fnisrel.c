/* fnisrel.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <ctype.h>

int _fnisrel (const char *name)
{
  return !(*name == '/' || *name == '\\'
           || (isalpha ((unsigned char)name[0]) && name[1] == ':'));
}
