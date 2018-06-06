/* mon5.c */

#define INCL_DOSMONITORS
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16MonReg) ();

USHORT DosMonReg (HMONITOR hmon, PBYTE pbInBuf, PBYTE pbOutBuf,
		  USHORT fPosition, USHORT usIndex)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+4+4+2+2);
           _THUNK_SHORT (hmon);
           _THUNK_FLAT (pbInBuf);
           _THUNK_FLAT (pbOutBuf);
           _THUNK_SHORT (fPosition);
           _THUNK_SHORT (usIndex);
           _THUNK_CALL (Dos16MonReg)));
}
