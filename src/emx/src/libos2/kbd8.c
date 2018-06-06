/* kbd8.c */

#define INCL_KBD
#include <os2.h>

USHORT _THUNK_FUNCTION (Kbd16Register) ();

USHORT KbdRegister (PCSZ pszModName, PCSZ pszEntryName, ULONG ulFunMask)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+4);
           _THUNK_FLAT ((PSZ)pszModName);
           _THUNK_FLAT ((PSZ)pszEntryName);
           _THUNK_LONG (ulFunMask);
           _THUNK_CALL (Kbd16Register)));
}
