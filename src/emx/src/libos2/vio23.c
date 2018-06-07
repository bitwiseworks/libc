/* vio23.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16ScrLock) ();

USHORT VioScrLock (USHORT fWait, PUCHAR pfNotLocked, HVIO hvio)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+4+2);
           _THUNK_SHORT (fWait);
           _THUNK_FLAT (pfNotLocked);
           _THUNK_SHORT (hvio);
           _THUNK_CALL (Vio16ScrLock)));
}
