/* setpgid.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

int _STD(setpgid) (pid_t pid, pid_t pgid)
{
  errno = ENOSYS;
  return -1;
}
