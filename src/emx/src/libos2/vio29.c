/* vio29.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetFont) ();

USHORT VioGetFont (PVIOFONTINFO pviofi, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pviofi);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16GetFont)));
}
