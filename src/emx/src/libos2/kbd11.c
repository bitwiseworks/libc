/* kbd11.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16SetStatus) ();

USHORT KbdSetStatus (PKBDINFO pkbdinfo, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pkbdinfo);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16SetStatus)));
}
