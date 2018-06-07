/* cfsetisp.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <termios.h>
#include <errno.h>

int _STD(cfsetispeed) (struct termios *ptermios, speed_t speed)
{
  return cfsetspeed (ptermios, speed);
}
