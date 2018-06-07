/* vio28.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16SavRedrawUndo) ();

USHORT VioSavRedrawUndo (USHORT usOwnerInd, USHORT usKillInd,
                         USHORT usReserved)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2);
           _THUNK_SHORT (usOwnerInd);
           _THUNK_SHORT (usKillInd);
           _THUNK_SHORT (usReserved);
           _THUNK_CALL (Vio16SavRedrawUndo)));
}
