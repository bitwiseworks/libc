/* mou1.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16GetPtrShape) ();

USHORT MouGetPtrShape (PBYTE pBuf, PPTRSHAPE pmoupsInfo, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+2);
           _THUNK_FLAT (pBuf);
           _THUNK_FLAT (pmoupsInfo);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16GetPtrShape)));
}
