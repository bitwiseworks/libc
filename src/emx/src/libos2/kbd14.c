/* kbd14.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16Xlate) ();

USHORT KbdXlate (PKBDTRANS pkbdtrans, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pkbdtrans);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16Xlate)));
}
