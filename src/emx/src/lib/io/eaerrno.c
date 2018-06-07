/* eaerrno.c (emx+gcc) -- Copyright (c) 1993 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#include <os2.h>
#include <stdlib.h>
#include <errno.h>
#include "ea.h"

void _ea_set_errno (ULONG rc)
{
  switch (rc)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      errno = ENOENT;
      break;
    case ERROR_ACCESS_DENIED:
      errno = EACCES;
      break;
    case ERROR_NOT_ENOUGH_MEMORY:
      errno = ENOMEM;
      break;
    case ERROR_INVALID_HANDLE:
      errno = EBADF;
      break;
    case ERROR_FILENAME_EXCED_RANGE:
      errno = ENAMETOOLONG;
      break;
    default:
      errno = EINVAL;
      break;
    }
}
