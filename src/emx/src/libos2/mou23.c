/* mou23.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16Synch) ();

USHORT MouSynch (USHORT fWait)
{
  return ((USHORT)
          (_THUNK_PROLOG (2);
           _THUNK_SHORT (fWait);
           _THUNK_CALL (Mou16Synch)));
}
