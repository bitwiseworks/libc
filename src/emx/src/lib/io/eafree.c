/* eafree.c (emx+gcc) -- Copyright (c) 1993 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <sys/ea.h>

void _ea_free (struct _ea *ptr)
{
  if (ptr->value != NULL)
    {
      free (ptr->value);
      ptr->value = NULL;
    }
}
