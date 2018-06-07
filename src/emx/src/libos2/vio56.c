/* vio56.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16CreatePS) ();

USHORT VioCreatePS (PHVPS phvps, SHORT sDepth, SHORT sWidth, SHORT sFormat,
                    SHORT sAttrs, HVPS hvpsReserved)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2+2+2+2+2);
           _THUNK_FLAT (phvps);
           _THUNK_SHORT (sDepth);
           _THUNK_SHORT (sWidth);
           _THUNK_SHORT (sFormat);
           _THUNK_SHORT (sAttrs);
           _THUNK_SHORT (hvpsReserved);
           _THUNK_CALL (Vio16CreatePS)));
}
