/* dos134.c */

#define INCL_DOSSIGNALS
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16SendSignal) ();

USHORT APIENTRY DosSendSignal (USHORT idProcess, USHORT usSigNumber)
{
  return ((USHORT)
          (_THUNK_PASCAL_PROLOG (2+2);
           _THUNK_PASCAL_SHORT (idProcess);
           _THUNK_PASCAL_SHORT (usSigNumber);
           _THUNK_PASCAL_CALL (Dos16SendSignal)));
}



