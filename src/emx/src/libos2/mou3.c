/* mou3.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16GetNumMickeys) ();

USHORT MouGetNumMickeys (PUSHORT pcMickeys, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pcMickeys);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16GetNumMickeys)));
}
