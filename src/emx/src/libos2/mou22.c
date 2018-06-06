/* mou22.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16GetDevStatus) ();

USHORT MouGetDevStatus (PUSHORT pfsDevStatus, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pfsDevStatus);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16GetDevStatus)));
}
