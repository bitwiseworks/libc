/* kbd9.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16StringIn) ();

USHORT KbdStringIn (PCH pch, PSTRINGINBUF pchIn, USHORT fWait, HKBD hkbd)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+2+2);
           _THUNK_FLAT (pch);
           _THUNK_FLAT (pchIn);
           _THUNK_SHORT (fWait);
           _THUNK_SHORT (hkbd);
           _THUNK_CALL (Kbd16StringIn)));
}
