/* kbd17.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16Close) ();


USHORT KbdClose (HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (2);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16Close)));
}
