/* vio46.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetConfig) ();

USHORT VioGetConfig (USHORT usConfigId, PVIOCONFIGINFO pvioin, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+4+2);
           _THUNK_SHORT (usConfigId);
           _THUNK_FLAT (pvioin);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16GetConfig)));
}
