/* kbd4.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16CharIn) ();

USHORT KbdCharIn (PKBDKEYINFO pkbci, USHORT fWait, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2+2);
           _THUNK_FLAT (pkbci);
           _THUNK_SHORT (fWait);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16CharIn)));
}
