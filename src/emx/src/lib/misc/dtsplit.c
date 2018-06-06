/* dtsplit.c (emx+gcc) -- Copyright (c) 1987-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/dirtree.h>
#include <emx/io.h>


int _dt_split (const char *src, char *dir, char *mask)
{
  unsigned char drive;
  char *p;
  char old_dir[MAXPATHLEN+1];
  char path_buf[MAXPATHLEN+1];
  char mask_buf[MAXPATHLEN+1];
  int saved_errno;
  size_t len;

  drive = _fngetdrive (src);
  if (drive != 0)
    {
      src += 2;
      if (*src == 0)
        src = ".";
    }
  else
    drive = _getdrive ();
  path_buf[0] = drive;
  path_buf[1] = ':';
  if (strlen (src) >= sizeof (path_buf) - 2)
    {
      errno = ENAMETOOLONG;
      return -1;
    }
  strcpy (path_buf + 2, src);
  strcpy (mask_buf, "*.*");
  old_dir[0] = drive;
  old_dir[1] = ':';
  if (_getcwd1 (old_dir+2, drive) != 0)
    return -1;
  saved_errno = errno;
  if (_chdir_os2 (path_buf) != 0)
    {
      errno = saved_errno;
      p = _getname (path_buf + 2);
      if (p == path_buf + 2)
        {
          strcpy (mask_buf, path_buf+2);
          path_buf[2] = '.';
          path_buf[3] = 0;
        }
      else
        {
          if (*p != 0)
            strcpy (mask_buf, p);
          if (p == path_buf + 3)
            path_buf[3] = 0;    /* Root directory */
          else
            p[-1] = 0;          /* Remove last name */
        }
      if (_chdir_os2 (path_buf) != 0)
        return -1;
    }
  if (strcmp (mask_buf, "..") == 0)
    {
      _chdir_os2 (old_dir);
      errno = EINVAL;
      return -1;
    }
  if (_getcwd1 (path_buf+2, drive) != 0)
    {
      saved_errno = errno;
      _chdir_os2 (old_dir);
      errno = saved_errno;
      return -1;
    }
  if (path_buf[3] != 0)
    {
      len = strlen (path_buf);
      if (_trslash (path_buf, len, 0))
        path_buf[len-1] = 0;
    }
  _chdir_os2 (old_dir);
  if (dir != NULL)
    strcpy (dir, path_buf);
  if (mask != NULL)
    strcpy (mask, mask_buf);
  return 0;
}
