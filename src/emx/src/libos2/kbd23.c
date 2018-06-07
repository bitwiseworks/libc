/* kbd23.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16Open) ();


USHORT KbdOpen (PHKBD phkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (4);
           _THUNK_FLAT (phkbd);
           _THUNK_CALL (Kbd16Open)));
}
