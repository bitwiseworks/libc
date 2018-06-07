/* vio63.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16SetOrg) ();

USHORT VioSetOrg (SHORT sRow, SHORT sColumn, HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2);
           _THUNK_SHORT (sRow);
           _THUNK_SHORT (sColumn);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16SetOrg)));
}
