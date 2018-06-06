/* flock.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <sys/fcntl.h>
#include <limits.h>
#include <errno.h>
#include <emx/io.h>

/** @todo update limit.h to FreeBSD 5.1 or later. */
#ifndef OFF_MAX
#define OFF_MAX LONG_MAX
#endif

/** @todo check that fcntl is ok for flock implementation, have seen
 * comments and #defines in both Linux and FreeBSD which hints that
 * this isn't exactly the same thing.
 * @todo check thread safety as I see FreeBSD is taking that into account.
 */
int _STD(flock) (int fd, int request)
{
  struct flock  flck;
  int           fcntl_request;

  /* init the struct with defaults */
  flck.l_start = 0;
  flck.l_len = OFF_MAX;
  flck.l_whence = SEEK_SET;
  flck.l_pid = 0;
  if (request & LOCK_NB)
    fcntl_request = F_SETLK;            /* don't block when locking */
  else
    fcntl_request = F_SETLKW;

  /* what to do */
  switch (request & (LOCK_SH | LOCK_EX | LOCK_UN))
    {
      /* shared file lock */
      case LOCK_SH:
          flck.l_type = F_RDLCK;
          break;
      /* exclusive file lock */
      case LOCK_EX:
          flck.l_type = F_WRLCK;
          break;

      /* unlock file */
      case LOCK_UN:
          flck.l_type = F_UNLCK;
          fcntl_request = F_SETLK;      /* doesn't hurt I think? */
          break;

      default:
          errno = EINVAL;
          return -1;
    }

  /* do work */
  return fcntl(fd, fcntl_request, &flck);
}

