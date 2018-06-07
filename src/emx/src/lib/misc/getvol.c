/* getvol.c (emx+gcc) -- Copyright (c) 1994-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <InnoTekLIBC/thread.h>
#include <emx/syscalls.h>
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_FSMACROS
#include <os2emx.h>

char *_getvol (char drive)
{
  __LIBC_PTHREAD pThrd = __libc_threadCurrent();
  FS_VAR();

  if (drive == 0)
    drive = _getdrive ();
  if (drive >= 'a' && drive <= 'z')
    drive += 'A' - 'a';
  if (!(drive >= 'A' && drive <= 'Z'))
    return NULL;

  ULONG rc;
  FSINFO fsinfo;

  bzero (&fsinfo, sizeof (fsinfo));
  FS_SAVE_LOAD();
  rc = DosQueryFSInfo (drive - 'A' + 1, FSIL_VOLSER, &fsinfo,
                       sizeof (fsinfo));
  FS_RESTORE();
  if (rc != 0)
    return NULL;
  return strcpy (pThrd->szVolLabelBuf, fsinfo.vol.szVolLabel);
}
