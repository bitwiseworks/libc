/* vio8.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16PrtSc) ();

USHORT VioPrtSc (HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16PrtSc)));
}
