/* vio5.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16SetAnsi) ();

USHORT VioSetAnsi (USHORT fAnsi, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2);
           _THUNK_SHORT (fAnsi);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16SetAnsi)));
}
