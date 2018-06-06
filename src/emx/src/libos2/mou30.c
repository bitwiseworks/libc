/* mou30.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16GetThreshold) ();

USHORT MouGetThreshold (PTHRESHOLD pthreshold, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pthreshold);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16GetThreshold)));
}
