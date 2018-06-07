/* kbd25.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16SetHWID) ();

USHORT KbdSetHWID (PKBDHWID pkbdhwid, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pkbdhwid);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16SetHWID)));
}
