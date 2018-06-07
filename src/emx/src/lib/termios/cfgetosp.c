/* cfgetosp.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <termios.h>
#include <sys/termio.h>         /* CBAUD */

speed_t _STD(cfgetospeed) (const struct termios *ptermios)
{
  return ptermios->c_cflag & CBAUD;
}
