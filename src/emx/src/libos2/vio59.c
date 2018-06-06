/* vio59.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetOrg) ();

USHORT VioGetOrg (PSHORT psRow, PSHORT psColumn, HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+2);
           _THUNK_FLAT (psRow);
           _THUNK_FLAT (psColumn);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16GetOrg)));
}
