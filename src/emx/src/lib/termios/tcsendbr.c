/* tcsendbr.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>

int _STD(tcsendbreak) (int handle, int duration)
{
  return ioctl (handle, TCSBRK, duration);
}
