/* tcsetpgr.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

int _STD(tcsetpgrp) (int fd, pid_t pgrp)
{
  errno = ENOSYS;
  return -1;
}
