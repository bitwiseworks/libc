/* vio9.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetCurPos) ();

USHORT VioGetCurPos (PUSHORT pusRow, PUSHORT pusColumn, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+2);
           _THUNK_FLAT (pusRow);
           _THUNK_FLAT (pusColumn);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16GetCurPos)));
}
