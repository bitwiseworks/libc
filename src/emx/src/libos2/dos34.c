/* dos34.c */

#define INCL_DOSMEMMGR
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16AllocSeg) ();

USHORT DosAllocSeg (USHORT cbSize, PSEL pSel, USHORT fsAlloc)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+4+2);
           _THUNK_SHORT (cbSize);
           _THUNK_FLAT (pSel);
           _THUNK_SHORT (fsAlloc);
           _THUNK_CALL (Dos16AllocSeg)));
}

