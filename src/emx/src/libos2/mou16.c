/* mou16.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16SetEventMask) ();

USHORT MouSetEventMask (PUSHORT pfsEvents, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pfsEvents);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16SetEventMask)));
}
