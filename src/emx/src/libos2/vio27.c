/* vio27.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetCurType) ();

USHORT VioGetCurType (PVIOCURSORINFO pvioCursorInfo, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pvioCursorInfo);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16GetCurType)));
}
