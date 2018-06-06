/* vio49.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetState) ();

USHORT VioGetState (PVOID pvioState, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pvioState);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16GetState)));
}
