/* vio1.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16EndPopUp) ();

USHORT VioEndPopUp (HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16EndPopUp)));
}
