/* vio47.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16ScrollDn) ();

USHORT VioScrollDn (USHORT usTopRow, USHORT usLeftCol, USHORT usBotRow,
                    USHORT usRightCol, USHORT cbLines, PBYTE pCell, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2+2+2+4+2);
           _THUNK_SHORT (usTopRow);
           _THUNK_SHORT (usLeftCol);
           _THUNK_SHORT (usBotRow);
           _THUNK_SHORT (usRightCol);
           _THUNK_SHORT (cbLines);
           _THUNK_FLAT (pCell);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16ScrollDn)));
}
