/* eadwrite.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <sys/ead.h>
#include "ea.h"

struct del_data
{
  int fea_alloc;
  int fea_used;
  ULONG *patch;
  PFEA2LIST fea_ptr;
};


static int _ead_del (struct _ead_data *ead, PDENA2 pdena, void *arg)
{
  int add;
  PFEA2 pfea;
  struct del_data *p;

  if (_ead_find (ead, pdena->szName) < 0)
    {
      p = arg;
      add = _EA_SIZE1 (pdena->cbName, 0);
      if (p->fea_used + add > p->fea_alloc)
        {
          p->fea_alloc += 512;          /* increment must be > add */
          p->fea_ptr = realloc (p->fea_ptr, p->fea_alloc);
          if (p->fea_ptr == NULL)
            return -1;
        }
      pfea = (PFEA2)((char *)p->fea_ptr + p->fea_used);
      pfea->oNextEntryOffset = add;
      pfea->fEA = 0;
      pfea->cbName = pdena->cbName;
      pfea->cbValue = 0;        /* Delete! */
      memcpy (pfea->szName, pdena->szName, pdena->cbName + 1);
      p->patch = &pfea->oNextEntryOffset;
      p->fea_used += add;
    }
  return 0;
}


int _ead_write (_ead ead, const char *path, int handle, int flags)
{
  if (!(flags & _EAD_MERGE))
    {
      struct del_data dd;

      dd.fea_used = sizeof (ULONG);
      dd.fea_alloc = 0;
      dd.fea_ptr = NULL;
      dd.patch = NULL;
      if (_ead_enum (ead, path, handle, _ead_del, &dd) < 0)
        {
          if (dd.fea_ptr != NULL)
            free (dd.fea_ptr);
          return -1;
        }
      if (dd.fea_ptr != NULL)
        {
          *dd.patch = 0;
          dd.fea_ptr->cbList = dd.fea_used;
          if (_ea_write (path, handle, dd.fea_ptr) < 0)
            {
              free (dd.fea_ptr);
              return -1;
            }
          free (dd.fea_ptr);
        }
    }
  if (ead->count != 0 && _ea_write (path, handle, ead->buffer) < 0)
    return -1;
  return 0;
}
