/* vio11.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16PopUp) ();

USHORT VioPopUp (PUSHORT pfWait, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pfWait);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16PopUp)));
}
