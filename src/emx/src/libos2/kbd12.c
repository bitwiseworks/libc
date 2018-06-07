/* kbd12.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16GetFocus) ();

USHORT KbdGetFocus (USHORT fWait, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2);
           _THUNK_SHORT (fWait);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16GetFocus)));
}
