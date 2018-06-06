/* vio51.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16SetState) ();

USHORT VioSetState (CPVOID pvioState, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT ((PVOID)pvioState);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16SetState)));
}
