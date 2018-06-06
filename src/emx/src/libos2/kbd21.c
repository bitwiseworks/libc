/* kbd21.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16SetFgnd) ();

USHORT KbdSetFgnd (VOID)
{
  return ((USHORT)
          (_THUNK_PROLOG (0);
           _THUNK_CALL (Kbd16SetFgnd)));
}
