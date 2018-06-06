/* mon1.c */

#define INCL_DOSMONITORS
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16MonWrite) ();

USHORT DosMonWrite (PBYTE pbOutBuf, PBYTE pbDataBuf, USHORT cbData)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+2);
           _THUNK_FLAT (pbOutBuf);
           _THUNK_FLAT (pbDataBuf);
           _THUNK_SHORT (cbData);
           _THUNK_CALL (Dos16MonWrite)));
}
