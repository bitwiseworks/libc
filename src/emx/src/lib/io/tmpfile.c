/* tmpfile.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */
/*                        Copyright (c) 1991-1993 by Kolja Elsaesser */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <emx/io.h>
#include "_tmp.h"


FILE *_STD(tmpfile) (void)
{
  char name[L_tmpnam];
  int idx, fd;
  FILE *f;

  do
    {
      idx = _tmpidxnam (name);
      if (idx == -1)
        return NULL;
      fd = open (name, O_RDWR|O_CREAT|O_EXCL|O_BINARY, 0644);
    } while (fd == -1 && errno == EEXIST);
  if (fd == -1)
    return NULL;
  f = fdopen (fd, "w+b");
  if (f == NULL)
    return NULL;
  f->_tmpidx = idx;
  f->_flags |= _IOTMP;
  return f;
}
