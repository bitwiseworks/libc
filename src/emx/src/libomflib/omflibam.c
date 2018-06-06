/* omflibam.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

/* Add an OBJ module to an OMFLIB. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "omflib0.h"
#include <sys/omflib.h>

int omflib_add_module (struct omflib *p, const char *fname, char *error)
{
  FILE *f;
  char name[256];
  char obj_fname[256+4];
  byte libhdr[1];
  int ret;

  omflib_module_name (name, fname);
  strcpy (obj_fname, fname);
  _defext (obj_fname, "obj");
  f = fopen (obj_fname, "rb");
  if (f == NULL)
    return omflib_set_error (error);

  /* this may be a library, check so we don't create invalid libraries 
     (#579?) */
  if (   fread(&libhdr[0], sizeof(libhdr), 1, f) == 1
      && libhdr[0] == LIBHDR
      )
    {
      struct omflib *   src = omflib_open (fname, error);
      if (!src)
          return -1;
      ret = omflib_copy_lib(p, src, error);
      omflib_close(src, error);
      return ret;
    }
  fseek(f, 0, SEEK_SET);

  ret = omflib_copy_module (p, p->f, NULL, f, name, error);
  fclose (f);
  return ret;
}
