/* tcgetpgr.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

pid_t _STD(tcgetpgrp) (int fd)
{
  errno = ENOSYS;
  return -1;
}
