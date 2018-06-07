/* vio22.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16SetMode) ();

USHORT VioSetMode (PVIOMODEINFO pvioModeInfo, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pvioModeInfo);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16SetMode)));
}
