/* eawrite.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#define INCL_FSMACROS
#include <os2.h>
#include <stdlib.h>
#include "ea.h"

int _ea_write (const char *path, int handle, PFEA2LIST src)
{
  ULONG rc;
  EAOP2 eaop;
  FS_VAR();

  eaop.fpGEA2List = NULL;
  eaop.fpFEA2List = src;
  eaop.oError = 0;
  FS_SAVE_LOAD();
  if (path != NULL)
    rc = DosSetPathInfo ((PCSZ)path, 2, &eaop, sizeof (eaop), 0);
  else
    rc = DosSetFileInfo (handle, 2, &eaop, sizeof (eaop));
  FS_RESTORE();
  if (rc != 0)
    {
      _ea_set_errno (rc);
      return -1;
    }
  return 0;
}
