/* tcgetatt.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <termios.h>
#include <sys/ioctl.h>

int _STD(tcgetattr) (int handle, struct termios *ptermios)
{
  return ioctl (handle, _TCGA, ptermios);
}
