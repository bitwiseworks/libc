/* vio55.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16Associate) ();

USHORT VioAssociate (HDC hdc, HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_LONG ((ULONG)hdc);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16Associate)));
}
