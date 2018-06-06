/* xf86sup.h -- Definitions for Holger Veit's xf86sup device driver
   Copyright (c) 1995-1996 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


#define IOCTL_XF86SUP           0x76

#define XF86SUP_TIOCSETA        0x48
#define XF86SUP_TIOCSETAW       0x49
#define XF86SUP_TIOCSETAF       0x4a
#define XF86SUP_TIOCFLUSH       0x4c
#define XF86SUP_TIOCDRAIN       0x4e
#define XF86SUP_ENADUP          0x5a
#define XF86SUP_NAME            0x60
#define XF86SUP_DRVID           0x61
#define XF86SUP_FIONREAD        0x64
#define XF86SUP_TIOCGETA        0x65
#define XF86SUP_FIONBIO         0x6a
#define XF86SUP_SELREG          0x6b
#define XF86SUP_SELARM          0x6c

#define XF86SUP_MAGIC           0x36384f58
#define XF86SUP_ID_PTY          1
#define XF86SUP_ID_TTY          2
#define XF86SUP_ID_CONSOLE      3
#define XF86SUP_ID_PMAP         4
#define XF86SUP_ID_FASTIO       5
#define XF86SUP_ID_PTMS         6

#define XF86SUP_SEL_READ        0x01
#define XF86SUP_SEL_EXCEPT      0x02

struct xf86sup_drvid
{
  ULONG magic;
  ULONG id;
  ULONG version;
};

struct xf86sup_selreg
{
  ULONG	rsel;
  ULONG	xsel;
  ULONG	code;
};

struct xf86sup_termios
{
  unsigned short c_iflag;
  unsigned short c_oflag;
  unsigned short c_cflag;
  unsigned short c_lflag;
  unsigned char  c_cc[NCCS];
  long           c_reserved[4];
};
