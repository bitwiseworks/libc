/* omflibex.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

/* Extract a module of an OMFLIB into an OBJ file. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "omflib0.h"
#include <sys/omflib.h>


int omflib_extract (struct omflib *p, const char *name, char *error)
{
  char buf[256+4];
  int page;
  FILE *f;

  if (p->dict == NULL && omflib_read_dictionary (p, error) != 0)
    return -1;
  page = omflib_find_module (p, name, error);
  if (page == 0)
    {
      strcpy (error, "Module not found");
      return -1;
    }
  _strncpy (buf, name, 255);
  _defext (buf, "obj");
  f = fopen (buf, "wb");
  if (f == NULL)
    return omflib_set_error (error);
  fseek (p->f, page * p->page_size, SEEK_SET);
  if (omflib_copy_module (NULL, f, p, p->f, NULL, error) != 0)
    {
      fclose (f);
      remove (buf);
      return -1;
    }
  if (fflush (f) != 0 || fclose (f) != 0)
    return omflib_set_error (error);
  return 0;
}
