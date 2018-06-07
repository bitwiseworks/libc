/* kbd5.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16SetCp) ();

USHORT KbdSetCp (USHORT usReserved, USHORT idCP, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2);
           _THUNK_SHORT (usReserved);
           _THUNK_SHORT (idCP);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16SetCp)));
}
