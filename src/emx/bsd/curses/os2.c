#include <io.h>
#include <fcntl.h>
#include <sys/termio.h>

#include "curses.h"


static struct termio tio;

int _setraw(int on)
{
  if ( !(on & 2) )
  {
    setmode(0, (on & 1) ? O_BINARY : O_TEXT);
    _pfast = (on & 1) || !(_tty.c_iflag & ICRNL);
  }
  _rawmode = (on & 1);
  _tty.c_lflag &= ~IDEFAULT;
  if ( (on & 1) )
    _tty.c_lflag &= ~ICANON;
  else
    _tty.c_lflag |= ICANON;
  ioctl(_tty_ch, TCSETA, &_tty);
  return 0;
}

int _setecho(int on)
{
  _echoit = on;
  if ( on )
    _tty.c_lflag |= ECHO;
  else
    _tty.c_lflag &= ~ECHO;
  ioctl (0, TCSETA, &_tty);
  return 0;
}

int _setnl(int on)
{
  _pfast = on ? _rawmode : TRUE;
  if ( on )
    _tty.c_iflag |= ICRNL;
  else
    _tty.c_iflag &= ~ICRNL;
  ioctl (0, TCSETA, &_tty);
  return 0;
}

savetty()
{
  ioctl (0, TCGETA, &tio);
  return 0;
}

resetty()
{
  ioctl (0, TCSETA, &tio);
  _echoit = (tio.c_lflag & ECHO) == ECHO;
  _rawmode = (tio.c_iflag & ICANON) == 0;
  _pfast = (tio.c_iflag & ICRNL) ? _rawmode : TRUE;
  return 0;
}

erasechar()
{
 return '\b';
}

killchar()
{
  return 127;
}

baudrate()
{
  return B38400;
}
