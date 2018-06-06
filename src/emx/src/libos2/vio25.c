/* vio25.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16SavRedrawWait) ();

USHORT VioSavRedrawWait (USHORT usRedrawInd, PUSHORT pusNotifyType,
                         USHORT usReserved)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+4+2);
           _THUNK_SHORT (usRedrawInd);
           _THUNK_FLAT (pusNotifyType);
           _THUNK_SHORT (usReserved);
           _THUNK_CALL (Vio16SavRedrawWait)));
}
