/* vio40.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetCp) ();

USHORT VioGetCp (USHORT usReserved, PUSHORT pusCodePage, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+4+2);
           _THUNK_SHORT (usReserved);
           _THUNK_FLAT (pusCodePage);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16GetCp)));
}
