/* mou21.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16SetPtrPos) ();

USHORT MouSetPtrPos (PPTRLOC pmouLoc, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pmouLoc);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16SetPtrPos)));
}
