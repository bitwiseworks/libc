/* getpass1.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <sys/ioctl.h>

char *_getpass1 (const char *prompt)
{
  int ht, kbd;

  kbd = (ioctl (0, FGETHTYPE, &ht) == 0 && ht == HT_DEV_CON);
  return _getpass2 (prompt, kbd);
}
