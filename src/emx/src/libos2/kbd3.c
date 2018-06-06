/* kbd3.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16GetCp) ();

USHORT KbdGetCp (ULONG ulReserved, PUSHORT pidCP, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+2);
           _THUNK_LONG (ulReserved);
           _THUNK_FLAT (pidCP);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16GetCp)));
}
