/* mou25.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16SetDevStatus) ();

USHORT MouSetDevStatus (PUSHORT pfsDevStatus, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pfsDevStatus);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16SetDevStatus)));
}
