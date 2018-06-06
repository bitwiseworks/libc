/* wrapper.h -- 32-bit wrappers for 16-bit functions
   Copyright (c) 1994-1995 by Eberhard Mattes

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


#include <os2thunk.h>

#define PFLG_A                  0

#define FLGP_SUBTREE            0
#define FLGP_PID                1

#define SIG_PFLG_A              5

#define SIGA_ACCEPT             2

USHORT DosFlagProcess (PID pid, USHORT fScope, USHORT usFlagNum,
    USHORT usFlagArg);
USHORT DosSetSigHandler (_far16ptr pfnSigHandler, _far16ptr *ppfnPrev,
    PUSHORT pfAction, USHORT fAction, USHORT usSigNum);
