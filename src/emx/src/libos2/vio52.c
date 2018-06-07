/* vio52.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16WrtNCell) ();

USHORT VioWrtNCell (const BYTE *pCell, USHORT cb, USHORT usRow,
                    USHORT usColumn, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2+2+2+2);
           _THUNK_FLAT ((PBYTE)pCell);
           _THUNK_SHORT (cb);
           _THUNK_SHORT (usRow);
           _THUNK_SHORT (usColumn);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16WrtNCell)));
}

