/* vio66.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16ShowPS) ();

USHORT VioShowPS (SHORT sDepth, SHORT sWidth, SHORT soffCell, HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2+2);
           _THUNK_SHORT (sDepth);
           _THUNK_SHORT (sWidth);
           _THUNK_SHORT (soffCell);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16ShowPS)));
}
