/* kbd7.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16Synch) ();

USHORT KbdSynch (USHORT fWait)
{
  return ((USHORT)
          (_THUNK_PROLOG (2);
           _THUNK_SHORT (fWait);
           _THUNK_CALL (Kbd16Synch)));
}
