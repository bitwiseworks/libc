/* eaget.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#define INCL_FSMACROS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <alloca.h>
#include <errno.h>
#include <sys/ea.h>
#include "ea.h"

int _ea_get (struct _ea *dst, const char *path, int handle,
             const char *name)
{
  ULONG rc;
  EAOP2 eaop;
  PGEA2LIST pgealist;
  PFEA2LIST pfealist;
  PGEA2 pgea;
  PFEA2 pfea;
  int len, size;
  FS_VAR();

  dst->flags = 0;
  dst->size = 0;
  dst->value = NULL;
  len = strlen (name);
  size = sizeof (GEA2LIST) + len;
  pgealist = alloca (size);
  pgealist->cbList = size;
  pgea = &pgealist->list[0];
  pgea->oNextEntryOffset = 0;
  pgea->cbName = len;
  memcpy (pgea->szName, name, len + 1);
  size = sizeof (FEA2LIST) + 0x10000;
  pfealist = alloca (size);
  pfealist->cbList = size;
  eaop.fpGEA2List = pgealist;
  eaop.fpFEA2List = pfealist;
  eaop.oError = 0;
  FS_SAVE_LOAD();
  if (path == NULL)
    rc = DosQueryFileInfo (handle, FIL_QUERYEASFROMLIST, &eaop,
                           sizeof (eaop));
  else
    rc = DosQueryPathInfo ((PCSZ)path, FIL_QUERYEASFROMLIST, &eaop,
                           sizeof (eaop));
  FS_RESTORE();
  if (rc != 0)
    {
      _ea_set_errno (rc);
      return -1;
    }
  pfea = &pfealist->list[0];
  if (pfea->cbValue != 0)
    {
      dst->value = malloc (pfea->cbValue);
      if (dst->value == NULL)
        {
          errno = ENOMEM;
          return -1;
        }
      memcpy (dst->value, pfea->szName + pfea->cbName + 1, pfea->cbValue);
    }
  dst->flags = pfea->fEA;
  dst->size = pfea->cbValue;
  return 0;
}
