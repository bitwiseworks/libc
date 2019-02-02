/* sys/filesys.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#define INCL_FSMACROS
#include <os2emx.h>
#include <alloca.h>
#include <emx/syscalls.h>
#include "syscalls.h"

#define QFSA_SIZE 256

int _filesys (const char *drive, char *name, size_t size)
{
  ULONG rc;
  ULONG len;
  FSQBUFFER2 *buf;
  FS_VAR();

  len = sizeof (FSQBUFFER2) + QFSA_SIZE;
  buf = alloca (len);
  FS_SAVE_LOAD();
  rc = DosQueryFSAttach ((PCSZ)drive, 1, FSAIL_QUERYNAME, buf, &len);
  FS_RESTORE();
  if (rc != 0)
    {
      _sys_set_errno (rc);
      return -1;
    }
  if (buf->iType != FSAT_LOCALDRV && buf->iType != FSAT_REMOTEDRV)
    {
      errno = EINVAL;
      return -1;
    }
  if (buf->cbFSDName >= size)
    {
      errno = E2BIG;
      return -1;
    }
  strcpy (name, (char *)buf->szFSDName + buf->cbName);
  return 0;
}
