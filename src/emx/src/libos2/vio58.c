/* vio58.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetDeviceCellSize) ();

USHORT VioGetDeviceCellSize (PSHORT psHeight, PSHORT psWidth, HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+2);
           _THUNK_FLAT (psHeight);
           _THUNK_FLAT (psWidth);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16GetDeviceCellSize)));
}
