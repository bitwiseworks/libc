/* emxloadu.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/emxload.h>
#include <emx/emxload.h>

int _emxload_unload (const char *name, int wait_flag)
{
  char buf1[MAXPATHLEN];
  char buf2[MAXPATHLEN];
  int req_code;

  _strncpy (buf1, name, sizeof (buf1));
  _defext (buf1, "exe");
  if (_path (buf2, buf1) != 0 || _abspath (buf2, buf2, sizeof (buf2)) != 0)
    return -1;
  req_code = (wait_flag ? _EMXLOAD_UNLOAD_WAIT : _EMXLOAD_UNLOAD);
  return _emxload_request (req_code, buf2, 0);
}
