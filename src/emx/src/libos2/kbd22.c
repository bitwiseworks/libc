/* kbd22.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16Peek) ();

USHORT KbdPeek (PKBDKEYINFO pkbci, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pkbci);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16Peek)));
}
