/* mou6.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16GetScaleFact) ();

USHORT MouGetScaleFact (PSCALEFACT pmouscFactors, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pmouscFactors);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16GetScaleFact)));
}
