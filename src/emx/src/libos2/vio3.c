/* vio3.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetAnsi) ();

USHORT VioGetAnsi (PUSHORT pfAnsi, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pfAnsi);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16GetAnsi)));
}
