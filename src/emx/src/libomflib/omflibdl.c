/* omflibdl.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

/* Mark a module of an OMFLIB as deleted. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "omflib0.h"
#include <sys/omflib.h>


int omflib_mark_deleted (struct omflib *p, const char *name, char *error)
{
  char buf[256];
  int (*compare)(const char *s1, const char *s2);
  int i;

  if (p->mod_count == -1 && omflib_make_mod_tab (p, error) != 0)
    return -1;
  omflib_module_name (buf, name);
  compare = (p->flags & 1 ? strcmp : stricmp);
  for (i = 0; i < p->mod_count; ++i)
    if (compare (p->mod_tab[i].name, buf) == 0)
      {
        p->mod_tab[i].flags |= FLAG_DELETED;
        return 0;
      }
  strcpy (error, "Module not found: ");
  strcat (error, buf);
  return 0;
}
