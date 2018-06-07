/* kbd1.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16SetCustXt) ();

USHORT KbdSetCustXt (PUSHORT pusCodePage, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pusCodePage);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16SetCustXt)));
}
