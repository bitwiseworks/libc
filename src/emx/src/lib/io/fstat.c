/* fstat.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <time.h>
#include <io.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <emx/time.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/backend.h>

int _STD(fstat) (int handle, struct stat *buffer)
{
  int rc = __libc_Back_fsFileStatFH (handle, buffer);
  if (rc == 0)
    {
      if (!_tzset_flag)
        tzset ();
      _loc2gmt (&buffer->st_atime, -1);
      _loc2gmt (&buffer->st_mtime, -1);
      _loc2gmt (&buffer->st_ctime, -1);
      _loc2gmt (&buffer->st_birthtime, -1);
    }
  else
    {
      errno = -rc;
      rc = -1;
    }
  return rc;
}
