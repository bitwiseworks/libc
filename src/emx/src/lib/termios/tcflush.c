/* tcflush.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <termios.h>
#include <sys/ioctl.h>

int _STD(tcflush) (int handle, int queue)
{
  return ioctl (handle, TCFLSH, queue);
}
