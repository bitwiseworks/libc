/* vio19.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16WrtTTY) ();

USHORT VioWrtTTY (PCCH pch, USHORT cb, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2+2);
           _THUNK_FLAT ((PCH)pch);
           _THUNK_SHORT (cb);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16WrtTTY)));
}
