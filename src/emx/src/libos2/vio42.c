/* vio42.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16SetCp) ();

USHORT VioSetCp (USHORT usReserved, USHORT usCodePage, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2);
           _THUNK_SHORT (usReserved);
           _THUNK_SHORT (usCodePage);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16SetCp)));
}
