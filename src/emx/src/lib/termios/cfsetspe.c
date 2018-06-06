/* cfsetspe.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <termios.h>
#include <errno.h>
#include <sys/termio.h>         /* CBAUD */

int _STD(cfsetspeed) (struct termios *ptermios, speed_t speed)
{
  /* speed is always >= B0 as speed_t is unsigned and B0 is zero. */

  if (speed > B38400)
    {
      errno = EINVAL;
      return -1;
    }
  ptermios->c_cflag &= ~CBAUD;
  ptermios->c_cflag |= speed & CBAUD;
  return 0;
}
