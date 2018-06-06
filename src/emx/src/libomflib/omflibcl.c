/* omflibcl.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

/* Copy an OMFLIB. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "omflib0.h"
#include <sys/omflib.h>


int omflib_copy_lib (struct omflib *dst, struct omflib *src, char *error)
{
  int i, rc, *reverse;
  long long_page;
  struct omf_rec rec;

  if (src->mod_count == -1 && omflib_make_mod_tab (src, error) != 0)
    return -1;

  /* Build a vector which maps page numbers to module table
     entries. */

  reverse = malloc (65536 * sizeof (*reverse));
  if (reverse == NULL)
    {
      errno = ENOMEM;
      return omflib_set_error (error);
    }

  for (i = 0; i < 65536; ++i)
    reverse[i] = -1;
  for (i = 0; i < src->mod_count; ++i)
    reverse[src->mod_tab[i].page] = i;

  /* Now read the source library sequentially, starting on page 1. */

  fseek (src->f, src->page_size, SEEK_SET);
  for (;;)
    {
      long_page = ftell (src->f) / src->page_size;
      if (long_page == -1)
        {
          omflib_set_error (error);
          free (reverse);
          return -1;
        }
      if (long_page > 65535)
        {
          strcpy (error, "Source library too big");
          free (reverse);
          return -1;
        }
      if (fread (&rec, sizeof (rec), 1, src->f) != 1)
        {
          if (ferror (src->f))
            omflib_set_error (error);
          else
            strcpy (error, "Unexpected end of file");
          free (reverse);
          return -1;
        }
      if (rec.rec_type == LIBEND)
        break;
      if (rec.rec_type != THEADR)
        {
          strcpy (error, "THEADR or LIBEND expected");
          free (reverse);
          return -1;
        }
      fseek (src->f, -sizeof (rec), SEEK_CUR);
      i = reverse[long_page];
      if (i == -1)
        {
          /* Unknown module, perhaps an import definition.  Copy the
             module. */
          rc = omflib_copy_module (dst, dst->f, src, src->f, NULL, error);
        }
      else if (src->mod_tab[i].flags & FLAG_DELETED)
        {
          /* Module marked for deletion.  Skip it. */
          rc = omflib_copy_module (NULL, NULL, src, src->f, NULL, error);
        }
      else
        {
          /* Copy the module. */
          rc = omflib_copy_module (dst, dst->f, src, src->f,
                                   src->mod_tab[i].name, error);
        }
      if (rc != 0)
        {
          free (reverse);
          return rc;
        }
    }
  free (reverse);
  return 0;
}
