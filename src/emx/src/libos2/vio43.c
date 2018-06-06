/* vio43.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16ShowBuf) ();

USHORT VioShowBuf (USHORT offLVB, USHORT cb, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2);
           _THUNK_SHORT (offLVB);
           _THUNK_SHORT (cb);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16ShowBuf)));
}
