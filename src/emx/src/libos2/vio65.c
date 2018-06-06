/* vio65.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16SetDeviceCellSize) ();

USHORT VioSetDeviceCellSize (SHORT sHeight, SHORT sWidth, HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2);
           _THUNK_SHORT (sHeight);
           _THUNK_SHORT (sWidth);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16SetDeviceCellSize)));
}
