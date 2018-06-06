/* strdup.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <stdlib.h>

char *_STD(strdup) (const char *string)
{
  char *p;
  size_t n;

  n = strlen (string) + 1;
  p = malloc (n);
  if (p != NULL)
    memcpy (p, string, n);
  return p;
}
