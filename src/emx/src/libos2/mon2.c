/* mon2.c */

#define INCL_DOSMONITORS
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16MonRead) ();

USHORT DosMonRead (PBYTE pbInBuf, USHORT fWait, PBYTE pbDataBuf,
                   PUSHORT pcbData)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2+4+4);
           _THUNK_FLAT (pbInBuf);
           _THUNK_SHORT (fWait);
           _THUNK_FLAT (pbDataBuf);
           _THUNK_FLAT (pcbData);
           _THUNK_CALL (Dos16MonRead)));
}
