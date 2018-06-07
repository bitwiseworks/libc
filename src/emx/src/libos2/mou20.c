/* mou20.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16ReadEventQue) ();

USHORT MouReadEventQue (PMOUEVENTINFO pmouevEvent, PUSHORT pfWait, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+2);
           _THUNK_FLAT (pmouevEvent);
           _THUNK_FLAT (pfWait);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16ReadEventQue)));
}
