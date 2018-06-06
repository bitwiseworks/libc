/* dos34.c */

#define INCL_DOSPROFILE
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16QProcStatus) ();

USHORT APIENTRY DosQProcStatus(PVOID pvBuffer, USHORT cbBuffer)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pvBuffer);
           _THUNK_SHORT (cbBuffer);
           _THUNK_CALL (Dos16QProcStatus)));
}

