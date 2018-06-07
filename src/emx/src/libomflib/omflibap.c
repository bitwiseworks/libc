/* omflibap.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

/* Declare a symbol public. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "omflib0.h"
#include <sys/omflib.h>

int omflib_add_pub (struct omflib *p, const char *name, word page, char *error)
{
  int i;

  if (strncmp (name, "__POST$", 7) == 0)
    return 0;

  if (p->pub_count >= p->pub_alloc)
    {
      p->pub_alloc += 32;
      p->pub_tab = realloc (p->pub_tab,
                            p->pub_alloc * sizeof (struct pubsym));
      if (p->pub_tab == NULL)
        {
          errno = ENOMEM;
          strcpy (error, strerror (errno));
          p->pub_alloc = 0;
          p->pub_count = 0;
          return -1;
        }
    }
  i = p->pub_count;
  if ((p->pub_tab[i].name = strdup (name)) == NULL)
    {
      errno = ENOMEM;
      return omflib_set_error (error);
    }
  p->pub_tab[i].page = page;
  ++p->pub_count;
  return 0;
}
