/* vio37.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16ModeWait) ();

USHORT VioModeWait (USHORT usReqType, PUSHORT pNotifyType, USHORT usReserved)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+4+2);
           _THUNK_SHORT (usReqType);
           _THUNK_FLAT (pNotifyType);
           _THUNK_SHORT (usReserved);
           _THUNK_CALL (Vio16ModeWait)));
}
