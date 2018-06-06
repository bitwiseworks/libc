/* vio45.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16Register) ();

USHORT VioRegister (PCSZ pszModName, PCSZ pszEntryName,
                    ULONG ulFunMask1, ULONG ulFunMask2)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+4+4);
           _THUNK_FLAT ((PSZ)pszModName);
           _THUNK_FLAT ((PSZ)pszEntryName);
           _THUNK_LONG (ulFunMask1);
           _THUNK_LONG (ulFunMask2);
           _THUNK_CALL (Vio16Register)));
}
