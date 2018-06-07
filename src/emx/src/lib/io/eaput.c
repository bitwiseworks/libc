/* eaput.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <alloca.h>
#include <errno.h>
#include <sys/ea.h>
#include "ea.h"

int _ea_put (struct _ea *src, const char *path, int handle,
             const char *name)
{
  PFEA2LIST pfealist;
  PFEA2 pfea;
  int len, size;

  len = strlen (name);
  size = sizeof (FEA2LIST) + len + src->size;
  pfealist = alloca (size);
  pfealist->cbList = size;
  pfea = &pfealist->list[0];
  pfea->oNextEntryOffset = 0;
  pfea->fEA = src->flags;
  pfea->cbName = len;
  pfea->cbValue = src->size;
  memcpy (pfea->szName, name, len + 1);
  memcpy (pfea->szName + len + 1, src->value, src->size);
  return _ea_write (path, handle, pfealist);
}
