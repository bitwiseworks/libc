/* vio75.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16CheckCharType) ();

USHORT VioCheckCharType (PUSHORT pType, USHORT usRow, USHORT usColumn,
                         HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2+2+2);
           _THUNK_FLAT (pType);
           _THUNK_SHORT (usRow);
           _THUNK_SHORT (usColumn);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16CheckCharType)));
}
