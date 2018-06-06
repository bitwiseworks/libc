/* tcsetatt.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>

int _STD(tcsetattr) (int handle, int options, const struct termios *ptermios)
{
  int request;

  switch (options)
    {
    case TCSANOW:
      request = _TCSANOW;
      break;
    case TCSADRAIN:
      request = _TCSADRAIN;
      break;
    case TCSAFLUSH:
      request = _TCSAFLUSH;
      break;
    default:
      errno = EINVAL;
      return -1;
    }
  return ioctl (handle, request, ptermios);
}
