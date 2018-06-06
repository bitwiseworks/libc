/* wrapper.c -- 32-bit wrappers for 16-bit functions
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


#include <os2emx.h>
#include "wrapper.h"

USHORT _THUNK_FUNCTION (Dos16FlagProcess) ();

USHORT DosFlagProcess (PID pid, USHORT fScope, USHORT usFlagNum,
                       USHORT usFlagArg)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2+2);
           _THUNK_SHORT (pid);
           _THUNK_SHORT (fScope);
           _THUNK_SHORT (usFlagNum);
           _THUNK_SHORT (usFlagArg);
           _THUNK_CALL (Dos16FlagProcess)));
}


USHORT _THUNK_FUNCTION (Dos16SetSigHandler) ();

USHORT DosSetSigHandler (_far16ptr pfnSigHandler, _far16ptr *ppfnPrev,
                         PUSHORT pfAction, USHORT fAction, USHORT usSigNum)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+4+2+2);
           _THUNK_FAR16 (pfnSigHandler);
           _THUNK_FLAT (ppfnPrev);
           _THUNK_FLAT (pfAction);
           _THUNK_SHORT (fAction);
           _THUNK_SHORT (usSigNum);
           _THUNK_CALL (Dos16SetSigHandler)));
}
