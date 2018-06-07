/* vio15.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16SetCurPos) ();

USHORT VioSetCurPos (USHORT usRow, USHORT usColumn, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2);
           _THUNK_SHORT (usRow);
           _THUNK_SHORT (usColumn);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16SetCurPos)));
}
