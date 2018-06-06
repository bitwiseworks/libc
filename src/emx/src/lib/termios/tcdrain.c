/* tcdrain.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <termios.h>
#include <sys/ioctl.h>

int _STD(tcdrain) (int handle)
{
  return ioctl (handle, TCSBRK, 1);
}
