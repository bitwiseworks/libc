/* mou19.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16GetPtrPos) ();

USHORT MouGetPtrPos (PPTRLOC pmouLoc, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pmouLoc);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16GetPtrPos)));
}
