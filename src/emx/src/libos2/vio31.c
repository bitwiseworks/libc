/* vio31.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetBuf) ();

USHORT VioGetBuf (PULONG pLVB, PUSHORT pcbLVB, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+2);
           _THUNK_FLAT (pLVB);
           _THUNK_FLAT (pcbLVB);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16GetBuf)));
}
