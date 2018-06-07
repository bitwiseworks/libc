/* dos38.c */

#define INCL_DOSMEMMGR
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16ReallocSeg) ();

USHORT APIENTRY DosReallocSeg(USHORT cbNewSize, SEL sel)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2);
           _THUNK_SHORT (cbNewSize);
           _THUNK_SHORT (sel);
           _THUNK_CALL (Dos16ReallocSeg)));
}
