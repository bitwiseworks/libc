/* cfsetosp.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <termios.h>

int _STD(cfsetospeed) (struct termios *ptermios, speed_t speed)
{
  return cfsetspeed (ptermios, speed);
}
