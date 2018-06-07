/* $Id: termios.h 3809 2014-02-16 20:20:59Z bird $ */
/** @file
 * EMX.
 */

#ifndef _SYS_TERMIOS_H_
#define _SYS_TERMIOS_H_

#include <sys/cdefs.h>


/* tcsetattr() */

#define TCSANOW     0
#define TCSADRAIN   1
#define TCSAFLUSH   2

/* tcflush() */

#define	TCIFLUSH    0
#define	TCOFLUSH    1
#define TCIOFLUSH   2

/* tcflow() */

#define	TCOOFF	    0
#define	TCOON	    1
#define TCIOFF	    2
#define TCION	    3

/* c_cc indexes */

#if !defined (VINTR)            /* Symbols common to termio.h and termios.h */
#define VINTR       0
#define VQUIT       1
#define VERASE      2
#define VKILL       3
#define VEOF        4
#define VEOL        5
#define VMIN        6
#define VTIME       7
#endif
#define VSUSP       8
#define VSTOP       9
#define VSTART      10

#define NCCS        11          /* Number of the above */

/* c_iflag, emx ignores most of the following bits */

#if !defined (IGNBRK)           /* Symbols common to termio.h and termios.h */
#define IGNBRK      0x0001
#define BRKINT      0x0002
#define IGNPAR      0x0004
#define PARMRK      0x0008
#define INPCK       0x0010
#define ISTRIP      0x0020
#define INLCR       0x0040
#define IGNCR       0x0080
#define ICRNL       0x0100
#define IXON        0x0400
#define IXOFF       0x1000
#if !defined (_POSIX_SOURCE)
#define IUCLC       0x0200      /* Extension */
#define IXANY       0x0800      /* Extension */
#define IDELETE     0x8000      /* Extension (emx) */
#endif
#endif

/* c_oflag, emx ignores all of the following bits */

#if !defined (OPOST)            /* Symbols common to termio.h and termios.h */
#define OPOST       0x0001
#endif

/* c_cflag, emx ignores all of the following bits */

#if !defined (B0)               /* Symbols common to termio.h and termios.h */
#define B0          0x0000
#define B50         0x0001
#define B75         0x0002
#define B110        0x0003
#define B134        0x0004
#define B150        0x0005
#define B200        0x0006
#define B300        0x0007
#define B600        0x0008
#define B1200       0x0009
#define B1800       0x000a
#define B2400       0x000b
#define B4800       0x000c
#define B9600       0x000d
#define B19200      0x000e
#define B38400      0x000f
#define CSIZE       0x0030      /* Mask */
#define CS5         0x0000
#define CS6         0x0010
#define CS7         0x0020
#define CS8         0x0030
#define CSTOPB      0x0040
#define CREAD       0x0080
#define PARENB      0x0100
#define PARODD      0x0200
#define HUPCL       0x0400
#define CLOCAL      0x0800
#if !defined (_POSIX_SOURCE)
#define LOBLK       0x1000      /* Extension */
#endif
#endif

/* c_lflag, emx ignores some of the following bits */

#if !defined (ISIG)             /* Symbols common to termio.h and termios.h */
#define ISIG        0x0001
#define ICANON      0x0002
#define ECHO        0x0008
#define ECHOE       0x0010
#define ECHOK       0x0020
#define ECHONL      0x0040
#define NOFLSH      0x0080
#if !defined (_POSIX_SOURCE)
#define XCASE       0x0004      /* Extension */
#endif
#endif
#define IEXTEN      0x0100
#define TOSTOP      0x0200


typedef unsigned char cc_t;
typedef unsigned int tcflag_t;
typedef unsigned int speed_t;

struct termios
{
  tcflag_t c_iflag;
  tcflag_t c_oflag;
  tcflag_t c_cflag;
  tcflag_t c_lflag;
  cc_t c_cc[NCCS];
  int c_reserved[4];
};

__BEGIN_DECLS
speed_t cfgetispeed (const struct termios *);
speed_t cfgetospeed (const struct termios *);
int cfsetispeed (struct termios *, speed_t);
int cfsetospeed (struct termios *, speed_t);
int tcdrain (int);
int tcflow (int, int);
int tcflush (int, int);
int tcgetattr (int, struct termios *);
int tcsendbreak (int, int);
int tcsetattr (int, int, const struct termios *);

#ifndef _POSIX_SOURCE
int	cfsetspeed (struct termios *, speed_t);
#endif /* !_POSIX_SOURCE */
__END_DECLS

#endif /* not _SYS_TERMIOS_H_ */
